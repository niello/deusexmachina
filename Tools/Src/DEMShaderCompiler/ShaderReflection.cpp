#include "ShaderReflection.h"

#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <Data/StringUtils.h>
#include <Render/RenderFwd.h>		// For GPU levels enum
#include <D3D9ShaderReflection.h>
#include <D3DCompiler.inl>

extern CString Messages;

bool D3D9CollectShaderMetadata(const void* pData, UPTR Size, const char* pSource, UPTR SourceSize, CSM30ShaderMeta& Out)
{
	CArray<CD3D9ConstantDesc> D3D9Consts;
	CString Creator;

	if (!D3D9Reflect(pData, Size, D3D9Consts, Creator)) FAIL;

	CDict<CString, CArray<CString>> SampToTex;
	D3D9FindSamplerTextures(pSource, SourceSize, SampToTex);

	CSM30ShaderBufferMeta* pMeta = Out.Buffers.Reserve(1);
	pMeta->Name = "$Global";

	for (UPTR i = 0; i < D3D9Consts.GetCount(); ++i)
	{
		CD3D9ConstantDesc& D3D9ConstDesc = D3D9Consts[i];

		if (D3D9ConstDesc.RegisterSet == RS_MIXED)
		{
			Messages += "    SM3.0 mixed-regset structs aren't supported, '";
			Messages += D3D9ConstDesc.Name;
			Messages += "' skipped\n";
			continue;
		}
		else if (D3D9ConstDesc.RegisterSet == RS_SAMPLER)
		{
			CSM30ShaderSamplerMeta* pMeta = Out.Samplers.Reserve(1);
			pMeta->Name = D3D9ConstDesc.Name;
			pMeta->RegisterStart = D3D9ConstDesc.RegisterIndex;
			pMeta->RegisterCount = D3D9ConstDesc.RegisterCount;

			switch (D3D9ConstDesc.Type.Type)
			{
				case PT_SAMPLER1D:		pMeta->Type = SM30Sampler_1D; break;
				case PT_SAMPLER3D:		pMeta->Type = SM30Sampler_3D; break;
				case PT_SAMPLERCUBE:	pMeta->Type = SM30Sampler_CUBE; break;
				default:				pMeta->Type = SM30Sampler_2D; break;
			}

			UPTR TexCount;
			int STIdx = SampToTex.FindIndex(D3D9ConstDesc.Name);
			if (STIdx == INVALID_INDEX) TexCount = 0;
			else
			{
				const CArray<CString>& TexNames = SampToTex.ValueAt(STIdx);
				TexCount = n_min(D3D9ConstDesc.RegisterCount, TexNames.GetCount());
				for (UPTR TexIdx = 0; TexIdx < TexCount; ++TexIdx)
				{
					const CString& TexName = TexNames[TexIdx];
					if (TexName.IsValid())
					{
						CSM30ShaderRsrcMeta* pMeta = Out.Resources.Reserve(1);
						pMeta->Name = TexName;
						pMeta->Register = D3D9ConstDesc.RegisterIndex + TexIdx;
					}
					else if (D3D9ConstDesc.RegisterCount > 1)
					{
						Messages += "Sampler '";
						Messages += D3D9ConstDesc.Name;
						Messages += '[';
						Messages += StringUtils::FromInt(TexIdx);
						Messages += "]' has no texture bound, use initializer in a form of 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'\n";
					}
					else
					{
						Messages += "Sampler '";
						Messages += D3D9ConstDesc.Name;
						Messages += "' has no texture bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }'\n";
					}
				}
			}

			if (!TexCount)
			{
				Messages += "Sampler '";
				Messages += D3D9ConstDesc.Name;
				Messages += "has no textures bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }'or 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'\n";
			}
		}
		else
		{
			// Structure layout is not saved for now, so operations on structure members aren't supported.
			// It can be fixed as needed.

			CSM30ShaderConstMeta* pMeta = Out.Consts.Reserve(1);
			pMeta->Name = D3D9ConstDesc.Name;
			pMeta->BufferIndex = 0; // Default, global buffer

			switch (D3D9ConstDesc.RegisterSet)
			{
				case RS_FLOAT4:	pMeta->RegisterSet = RS_Float4; break;
				case RS_INT4:	pMeta->RegisterSet = RS_Int4; break;
				case RS_BOOL:	pMeta->RegisterSet = RS_Bool; break;
				default:		Sys::Error("Unsupported SM3.0 register set %d\n", D3D9ConstDesc.RegisterSet); FAIL;
			};

			pMeta->RegisterStart = D3D9ConstDesc.RegisterIndex;
			pMeta->ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			pMeta->ElementCount = D3D9ConstDesc.Type.Elements;

			// Cache value
			pMeta->RegisterCount = pMeta->ElementRegisterCount * pMeta->ElementCount;

			n_assert(pMeta->RegisterCount == D3D9ConstDesc.RegisterCount);

			CSM30ShaderBufferMeta& BufMeta = Out.Buffers[pMeta->BufferIndex];
			CArray<UPTR>& UsedRegs = (pMeta->RegisterSet == RS_Float4) ? BufMeta.UsedFloat4 : ((pMeta->RegisterSet == RS_Int4) ? BufMeta.UsedInt4 : BufMeta.UsedBool);
			for (UPTR r = D3D9ConstDesc.RegisterIndex; r < D3D9ConstDesc.RegisterIndex + D3D9ConstDesc.RegisterCount; ++r)
			{
				if (!UsedRegs.Contains(r)) UsedRegs.Add(r);
			}
		}
	}

	UPTR i = 0;
	while (i < Out.Buffers.GetCount())
	{
		CSM30ShaderBufferMeta& B = Out.Buffers[i];
		if (!B.UsedFloat4.GetCount() &&
			!B.UsedInt4.GetCount() &&
			!B.UsedBool.GetCount())
		{
			Out.Buffers.RemoveAt(i);
		}
		else ++i;
	};

	OK;
}
//---------------------------------------------------------------------

bool D3D11CollectShaderMetadata(const void* pData, UPTR Size, CD3D11ShaderMeta& Out)
{
	ID3D11ShaderReflection* pReflector = NULL;

	if (FAILED(D3D11Reflect(pData, Size, &pReflector))) FAIL;

	D3D_FEATURE_LEVEL D3DFeatureLevel;
	if (FAILED(pReflector->GetMinFeatureLevel(&D3DFeatureLevel)))
	{
		pReflector->Release();
		FAIL;
	}

	switch (D3DFeatureLevel)
	{
		case D3D_FEATURE_LEVEL_9_1:		Out.MinFeatureLevel = Render::GPU_Level_D3D9_1; break;
		case D3D_FEATURE_LEVEL_9_2:		Out.MinFeatureLevel = Render::GPU_Level_D3D9_2; break;
		case D3D_FEATURE_LEVEL_9_3:		Out.MinFeatureLevel = Render::GPU_Level_D3D9_3; break;
		case D3D_FEATURE_LEVEL_10_0:	Out.MinFeatureLevel = Render::GPU_Level_D3D10_0; break;
		case D3D_FEATURE_LEVEL_10_1:	Out.MinFeatureLevel = Render::GPU_Level_D3D10_1; break;
		case D3D_FEATURE_LEVEL_11_0:	Out.MinFeatureLevel = Render::GPU_Level_D3D11_0; break;
		//case D3D_FEATURE_LEVEL_11_1:
		default:						Out.MinFeatureLevel = Render::GPU_Level_D3D11_1; break;
	}

	Out.RequiresFlags = pReflector->GetRequiresFlags();

	D3D11_SHADER_DESC D3DDesc;
	if (FAILED(pReflector->GetDesc(&D3DDesc)))
	{
		pReflector->Release();
		FAIL;
	}

	for (UINT RsrcIdx = 0; RsrcIdx < D3DDesc.BoundResources; ++RsrcIdx)
	{
		D3D11_SHADER_INPUT_BIND_DESC RsrcDesc;
		if (FAILED(pReflector->GetResourceBindingDesc(RsrcIdx, &RsrcDesc)))
		{
			pReflector->Release();
			FAIL;
		}

		// D3D_SIF_USERPACKED - may fail assertion if not set!

		switch (RsrcDesc.Type)
		{
			case D3D_SIT_TEXTURE:
			{
				CSM40ShaderRsrcMeta* pMeta = Out.Resources.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->RegisterStart = RsrcDesc.BindPoint;
				pMeta->RegisterCount = RsrcDesc.BindCount;

				switch (RsrcDesc.Dimension)
				{
					case D3D_SRV_DIMENSION_TEXTURE1D:			pMeta->Type = SM40Rsrc_Texture1D; break;
					case D3D_SRV_DIMENSION_TEXTURE1DARRAY:		pMeta->Type = SM40Rsrc_Texture1DArray; break;
					case D3D_SRV_DIMENSION_TEXTURE2D:			pMeta->Type = SM40Rsrc_Texture2D; break;
					case D3D_SRV_DIMENSION_TEXTURE2DARRAY:		pMeta->Type = SM40Rsrc_Texture2DArray; break;
					case D3D_SRV_DIMENSION_TEXTURE2DMS:			pMeta->Type = SM40Rsrc_Texture2DMS; break;
					case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:	pMeta->Type = SM40Rsrc_Texture2DMSArray; break;
					case D3D_SRV_DIMENSION_TEXTURE3D:			pMeta->Type = SM40Rsrc_Texture3D; break;
					case D3D_SRV_DIMENSION_TEXTURECUBE:			pMeta->Type = SM40Rsrc_TextureCUBE; break;
					case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:	pMeta->Type = SM40Rsrc_TextureCUBEArray; break;
					default:									pMeta->Type = SM40Rsrc_Unknown; break;
				}

				break;
			}
			case D3D_SIT_SAMPLER:
			{
				// D3D_SIF_COMPARISON_SAMPLER
				CSM40ShaderSamplerMeta* pMeta = Out.Samplers.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->RegisterStart = RsrcDesc.BindPoint;
				pMeta->RegisterCount = RsrcDesc.BindCount;
				break;
			}
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER:
			case D3D_SIT_STRUCTURED:
			{
				// Can't violate this condition (can't define cbuffer / tbuffer array)
				n_assert(RsrcDesc.BindCount == 1);

				// There can be cbuffer and tbuffer with the same name, so search by name and type
				D3D_CBUFFER_TYPE DesiredType;
				switch (RsrcDesc.Type)
				{
					case D3D_SIT_CBUFFER:		DesiredType = D3D_CT_CBUFFER; break;
					case D3D_SIT_TBUFFER:		DesiredType = D3D_CT_TBUFFER; break;
					case D3D_SIT_STRUCTURED:	DesiredType = D3D_CT_RESOURCE_BIND_INFO; break;
				}

				ID3D11ShaderReflectionConstantBuffer* pCB = NULL;
				D3D11_SHADER_BUFFER_DESC D3DBufDesc;
				for (UPTR BufIdx = 0; BufIdx < D3DDesc.ConstantBuffers; ++BufIdx)
				{
					ID3D11ShaderReflectionConstantBuffer* pCurrCB = pReflector->GetConstantBufferByIndex(BufIdx);
					pCurrCB->GetDesc(&D3DBufDesc);
					if (!strcmp(RsrcDesc.Name, D3DBufDesc.Name) && D3DBufDesc.Type == DesiredType)
					{
						pCB = pCurrCB;
						break;
					}
				}
				
				if (!pCB || !D3DBufDesc.Variables) continue;

				//D3DBufDesc.uFlags & D3D_CBF_USERPACKED

				DWORD TypeMask;
				if (RsrcDesc.Type == D3D_SIT_TBUFFER) TypeMask = D3D11Buffer_Texture;
				else if (RsrcDesc.Type == D3D_SIT_STRUCTURED) TypeMask = D3D11Buffer_Structured;
				else TypeMask = 0;

				CD3D11ShaderBufferMeta* pMeta = Out.Buffers.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = (RsrcDesc.BindPoint | TypeMask);
				pMeta->Size = D3DBufDesc.Size;

				// Structured buffer has only '$Element' structure, we don't process it now.
				// It may be useful if we will add support of structure layouts/members.
				if (RsrcDesc.Type != D3D_SIT_STRUCTURED)
				{
					for (UINT VarIdx = 0; VarIdx < D3DBufDesc.Variables; ++VarIdx)
					{
						ID3D11ShaderReflectionVariable* pVar = pCB->GetVariableByIndex(VarIdx);
						if (!pVar) continue;

						D3D11_SHADER_VARIABLE_DESC D3DVarDesc;
						pVar->GetDesc(&D3DVarDesc);

						//D3D_SVF_USERPACKED             = 1,
						//D3D_SVF_USED                   = 2,

						ID3D11ShaderReflectionType* pVarType = pVar->GetType();
						if (!pVarType) continue;

						D3D11_SHADER_TYPE_DESC D3DTypeDesc;
						pVarType->GetDesc(&D3DTypeDesc);

						ED3D11ConstType Type;
						if (D3DTypeDesc.Class == D3D_SVC_STRUCT)
						{
							Type = D3D11Const_Struct; // D3DTypeDesc.Type is 'void'
						}
						else
						{
							switch (D3DTypeDesc.Type)
							{
								case D3D_SVT_BOOL:	Type = D3D11Const_Bool; break;
								case D3D_SVT_INT:	Type = D3D11Const_Int; break;
								case D3D_SVT_FLOAT:	Type = D3D11Const_Float; break;
								default:
								{
									Messages += "Unsupported constant '";
									Messages += D3DVarDesc.Name;
									Messages += "' type '";
									Messages += StringUtils::FromInt(D3DTypeDesc.Type);
									Messages += "' in SM4.0+ buffer '";
									Messages += RsrcDesc.Name;
									Messages += "'\n";
									FAIL;
								}
							}
						}

						CD3D11ShaderConstMeta* pConstMeta = Out.Consts.Reserve(1);
						pConstMeta->Name = D3DVarDesc.Name;
						pConstMeta->BufferIndex = Out.Buffers.GetCount() - 1;
						pConstMeta->Type = Type;
						pConstMeta->Offset = D3DVarDesc.StartOffset;

						if (D3DTypeDesc.Elements > 1)
						{
							// Arrays
							pConstMeta->ElementSize = D3DVarDesc.Size / D3DTypeDesc.Elements;
							pConstMeta->ElementCount = D3DTypeDesc.Elements;
						}
						else
						{
							// Non-arrays and arrays [1]
							pConstMeta->ElementSize = D3DVarDesc.Size;
							pConstMeta->ElementCount = 1;
						}
					}
				}

				break;
			}
		}
	}

	pReflector->Release();

	OK;
}
//---------------------------------------------------------------------

void WriteRegisterRanges(const CArray<UPTR>& UsedRegs, IO::CBinaryWriter& W, const char* pRegisterSetName)
{
	U64 RangeCountOffset = W.GetStream().GetPosition();
	W.Write<U32>(0);

	if (!UsedRegs.GetCount()) return;

	U32 RangeCount = 0;
	UPTR CurrStart = UsedRegs[0], CurrCount = 1;
	for (UPTR r = 1; r < UsedRegs.GetCount(); ++r)
	{
		UPTR Reg = UsedRegs[r];
		if (Reg == CurrStart + CurrCount) ++CurrCount;
		else
		{
			// New range detected
			W.Write<U32>(CurrStart);
			W.Write<U32>(CurrCount);
			++RangeCount;
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	if (CurrStart != (UPTR)-1)
	{
		W.Write<U32>(CurrStart);
		W.Write<U32>(CurrCount);
		++RangeCount;
	}

	U64 EndOffset = W.GetStream().GetPosition();
	W.GetStream().Seek(RangeCountOffset, IO::Seek_Begin);
	W.Write<U32>(RangeCount);
	W.GetStream().Seek(EndOffset, IO::Seek_Begin);
}
//---------------------------------------------------------------------

bool D3D9SaveShaderMetadata(IO::CBinaryWriter& W, const CSM30ShaderMeta& Meta)
{
	W.Write<U32>(Meta.Buffers.GetCount());
	for (UPTR i = 0; i < Meta.Buffers.GetCount(); ++i)
	{
		CSM30ShaderBufferMeta& Obj = Meta.Buffers[i];
		Obj.UsedFloat4.Sort();
		Obj.UsedInt4.Sort();
		Obj.UsedBool.Sort();

		W.Write(Obj.Name);

		WriteRegisterRanges(Obj.UsedFloat4, W, "float4");
		WriteRegisterRanges(Obj.UsedInt4, W, "int4");
		WriteRegisterRanges(Obj.UsedBool, W, "bool");
	}

	W.Write<U32>(Meta.Consts.GetCount());
	for (UPTR i = 0; i < Meta.Consts.GetCount(); ++i)
	{
		const CSM30ShaderConstMeta& Obj = Meta.Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write<U8>(Obj.RegisterSet);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.ElementRegisterCount);
		W.Write(Obj.ElementCount);
	}

	W.Write<U32>(Meta.Resources.GetCount());
	for (UPTR i = 0; i < Meta.Resources.GetCount(); ++i)
	{
		const CSM30ShaderRsrcMeta& Obj = Meta.Resources[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
	}

	W.Write<U32>(Meta.Samplers.GetCount());
	for (UPTR i = 0; i < Meta.Samplers.GetCount(); ++i)
	{
		const CSM30ShaderSamplerMeta& Obj = Meta.Samplers[i];
		W.Write(Obj.Name);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

bool D3D11SaveShaderMetadata(IO::CBinaryWriter& W, const CD3D11ShaderMeta& Meta)
{
	W.Write<U32>(Meta.MinFeatureLevel);
	W.Write<U64>(Meta.RequiresFlags);

	W.Write<U32>(Meta.Buffers.GetCount());
	for (UPTR i = 0; i < Meta.Buffers.GetCount(); ++i)
	{
		const CD3D11ShaderBufferMeta& Obj = Meta.Buffers[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		W.Write(Obj.Size);
		//W.Write(Obj.ElementCount);
	}

	W.Write<U32>(Meta.Consts.GetCount());
	for (UPTR i = 0; i < Meta.Consts.GetCount(); ++i)
	{
		const CD3D11ShaderConstMeta& Obj = Meta.Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.Offset);
		W.Write(Obj.ElementSize);
		W.Write(Obj.ElementCount);
	}

	W.Write<U32>(Meta.Resources.GetCount());
	for (UPTR i = 0; i < Meta.Resources.GetCount(); ++i)
	{
		const CSM40ShaderRsrcMeta& Obj = Meta.Resources[i];
		W.Write(Obj.Name);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	W.Write<U32>(Meta.Samplers.GetCount());
	for (UPTR i = 0; i < Meta.Samplers.GetCount(); ++i)
	{
		const CSM40ShaderSamplerMeta& Obj = Meta.Samplers[i];
		W.Write(Obj.Name);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

void ReadRegisterRanges(CArray<UPTR>& UsedRegs, IO::CBinaryReader& R)
{
	UsedRegs.Clear();

	U32 RangeCount = 0;
	R.Read<U32>(RangeCount);
	for (UPTR i = 0; i < RangeCount; ++i)
	{
		U32 Curr;
		U32 CurrCount;
		R.Read<U32>(Curr);
		R.Read<U32>(CurrCount);
		U32 CurrEnd = Curr + CurrCount;
		for (; Curr < CurrEnd; ++Curr)
			UsedRegs.Add((UPTR)Curr);
	}
}
//---------------------------------------------------------------------

bool D3D9LoadShaderMetadata(IO::CBinaryReader& R, CSM30ShaderMeta& Meta)
{
	Meta.Buffers.Clear();
	Meta.Consts.Clear();
	Meta.Samplers.Clear();

	U32 Count;

	R.Read<U32>(Count);
	CSM30ShaderBufferMeta* pBuf = Meta.Buffers.Reserve(Count, false);
	for (; pBuf < Meta.Buffers.End(); ++pBuf)
	{
		CSM30ShaderBufferMeta& Obj = *pBuf;

		R.Read(Obj.Name);

		ReadRegisterRanges(Obj.UsedFloat4, R);
		ReadRegisterRanges(Obj.UsedInt4, R);
		ReadRegisterRanges(Obj.UsedBool, R);
	}

	R.Read<U32>(Count);
	CSM30ShaderConstMeta* pConst = Meta.Consts.Reserve(Count, false);
	for (; pConst < Meta.Consts.End(); ++pConst)
	{
		CSM30ShaderConstMeta& Obj = *pConst;

		U8 RegSet;

		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);
		R.Read<U8>(RegSet);

		switch (RegSet)
		{
			case 0:		Obj.RegisterSet = RS_Float4; break;
			case 1:		Obj.RegisterSet = RS_Int4; break;
			case 2:		Obj.RegisterSet = RS_Bool; break;
			default:	continue;
		};

		R.Read(Obj.RegisterStart);
		R.Read(Obj.ElementRegisterCount);
		R.Read(Obj.ElementCount);

		// Cache value
		Obj.RegisterCount = Obj.ElementRegisterCount * Obj.ElementCount;
	}

	R.Read<U32>(Count);
	CSM30ShaderRsrcMeta* pRsrc = Meta.Resources.Reserve(Count, false);
	for (; pRsrc < Meta.Resources.End(); ++pRsrc)
	{
		CSM30ShaderRsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		//???store sampler type or index for texture type validation on set?
		//???how to reference texture object in SM3.0 shader for it to be included in params list?
	}

	R.Read<U32>(Count);
	CSM30ShaderSamplerMeta* pSamp = Meta.Samplers.Reserve(Count, false);
	for (; pSamp < Meta.Samplers.End(); ++pSamp)
	{
		CSM30ShaderSamplerMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		
		U8 Type;
		R.Read<U8>(Type);
		Obj.Type = (ESM30SamplerType)Type;
		
		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

bool D3D11LoadShaderMetadata(IO::CBinaryReader& R, CD3D11ShaderMeta& Meta)
{
	Meta.Buffers.Clear();
	Meta.Consts.Clear();
	Meta.Resources.Clear();
	Meta.Samplers.Clear();

	R.Read<U32>(Meta.MinFeatureLevel);
	R.Read<U64>(Meta.RequiresFlags);

	U32 Count;

	R.Read<U32>(Count);
	CD3D11ShaderBufferMeta* pBuf = Meta.Buffers.Reserve(Count, false);
	for (; pBuf < Meta.Buffers.End(); ++pBuf)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		CD3D11ShaderBufferMeta& Obj = *pBuf;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		R.Read(Obj.Size);
		///R.Read(Obj.ElementCount);
	}

	R.Read<U32>(Count);
	CD3D11ShaderConstMeta* pConst = Meta.Consts.Reserve(Count, false);
	for (; pConst < Meta.Consts.End(); ++pConst)
	{
		CD3D11ShaderConstMeta& Obj = *pConst;
		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);

		U8 Type;
		R.Read<U8>(Type);
		Obj.Type = (ED3D11ConstType)Type;

		R.Read(Obj.Offset);
		R.Read(Obj.ElementSize);
		R.Read(Obj.ElementCount);
	}

	R.Read<U32>(Count);
	CSM40ShaderRsrcMeta* pRsrc = Meta.Resources.Reserve(Count, false);
	for (; pRsrc < Meta.Resources.End(); ++pRsrc)
	{
		CSM40ShaderRsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);

		U8 Type;
		R.Read<U8>(Type);
		Obj.Type = (ESM40ResourceType)Type;

		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	R.Read<U32>(Count);
	CSM40ShaderSamplerMeta* pSamp = Meta.Samplers.Reserve(Count, false);
	for (; pSamp < Meta.Samplers.End(); ++pSamp)
	{
		CSM40ShaderSamplerMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------
