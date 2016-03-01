#include "ShaderReflection.h"

#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <ConsoleApp.h>				// For n_msg
#include <D3D9ShaderReflection.h>
#include <D3DCompiler.inl>

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
			n_msg(VL_WARNING, "    SM3.0 mixed-regset structs aren't supported, '%s' skipped\n", D3D9ConstDesc.Name.CStr());
			continue;
		}
		else if (D3D9ConstDesc.RegisterSet == RS_SAMPLER)
		{
			CSM30ShaderSamplerMeta* pMeta = Out.Samplers.Reserve(1);
			pMeta->Name = D3D9ConstDesc.Name;
			pMeta->RegisterStart = D3D9ConstDesc.RegisterIndex;
			pMeta->RegisterCount = D3D9ConstDesc.RegisterCount;

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
						n_msg(VL_WARNING, "Sampler '%s[%d]' has no texture bound, use initializer in a form of 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'\n", D3D9ConstDesc.Name.CStr(), TexIdx);
					}
					else
					{
						n_msg(VL_WARNING, "Sampler '%s' has no texture bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }'\n", D3D9ConstDesc.Name.CStr());
					}
				}
			}

			if (!TexCount)
			{
				n_msg(VL_WARNING, "Sampler '%s' has no textures bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }'or 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'\n", D3D9ConstDesc.Name.CStr());
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

	D3D_FEATURE_LEVEL MinFeatureLevel;
	if (FAILED(pReflector->GetMinFeatureLevel(&MinFeatureLevel)))
	{
		pReflector->Release();
		FAIL;
	}
	Out.MinFeatureLevel = MinFeatureLevel;

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
				n_assert(RsrcDesc.BindCount == 1);
				CD3D11ShaderRsrcMeta* pMeta = Out.Resources.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = RsrcDesc.BindPoint; //???add array support? BindCount, TextureArray 1 bind point or not?
				break;
			}
			case D3D_SIT_SAMPLER:
			{
				// D3D_SIF_COMPARISON_SAMPLER
				n_assert(RsrcDesc.BindCount == 1);
				CD3D11ShaderRsrcMeta* pMeta = Out.Samplers.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = RsrcDesc.BindPoint; //???add array support? BindCount
				break;
			}
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER: //!!!resource, not cb! Var address = resource register (up to 128!) and Offset (big)
			case D3D_SIT_STRUCTURED: //!!!resource, not cb! Var address = resource register (up to 128!) and Offset (big)
			{
				n_assert(RsrcDesc.BindCount == 1); //???support arrays?

				ID3D11ShaderReflectionConstantBuffer* pCB = pReflector->GetConstantBufferByName(RsrcDesc.Name);
				if (!pCB) continue;

				D3D11_SHADER_BUFFER_DESC D3DBufDesc;
				pCB->GetDesc(&D3DBufDesc);
				if (!D3DBufDesc.Variables) continue;

				DWORD TypeMask;
				if (RsrcDesc.Type == D3D_SIT_TBUFFER) TypeMask = D3D11Buffer_Texture;
				else if (RsrcDesc.Type == D3D_SIT_STRUCTURED) TypeMask = D3D11Buffer_Structured;
				else TypeMask = 0;

				n_assert_dbg(!TypeMask); //!!!add tbuffer & sbuffer support!

				CD3D11ShaderBufferMeta* pMeta = Out.Buffers.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = (RsrcDesc.BindPoint | TypeMask);
				pMeta->ElementSize = D3DBufDesc.Size;
				pMeta->ElementCount = 1;

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
					if (D3DTypeDesc.Class == D3D_SVC_STRUCT) Type = D3D11Const_Struct;
					else
					{
						switch (D3DTypeDesc.Type)
						{
							case D3D_SVT_BOOL:	Type = D3D11Const_Bool; break;
							case D3D_SVT_INT:	Type = D3D11Const_Int; break;
							case D3D_SVT_FLOAT:	Type = D3D11Const_Float; break;
							default:
							{
								n_msg(VL_ERROR, "Unsupported constant '%s' type '%d' in SM4.0+ buffer '%s'\n", D3DVarDesc.Name, D3DTypeDesc.Type, RsrcDesc.Name);
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
			if (pRegisterSetName) n_msg(VL_DETAILS, "    Range: %s %d to %d\n", pRegisterSetName, CurrStart, CurrStart + CurrCount - 1);
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	if (CurrStart != (UPTR)-1)
	{
		W.Write<U32>(CurrStart);
		W.Write<U32>(CurrCount);
		++RangeCount;
		if (pRegisterSetName) n_msg(VL_DETAILS, "    Range: %s %d to %d\n", pRegisterSetName, CurrStart, CurrStart + CurrCount - 1);
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

		n_msg(VL_DETAILS, "    CBuffer: %s\n", Obj.Name.CStr());
	}

	W.Write<U32>(Meta.Consts.GetCount());
	for (UPTR i = 0; i < Meta.Consts.GetCount(); ++i)
	{
		const CSM30ShaderConstMeta& Obj = Meta.Consts[i];

		U8 RegSet;
		switch (Obj.RegisterSet)
		{
			case RS_Float4:	RegSet = 0; break;
			case RS_Int4:	RegSet = 1; break;
			case RS_Bool:	RegSet = 2; break;
			default:		continue;
		};

		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write<U8>(RegSet);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.ElementRegisterCount);
		W.Write(Obj.ElementCount);

		const char* pRegisterSetName = NULL;
		switch (Obj.RegisterSet)
		{
			case RS_Float4:	pRegisterSetName = "float4"; break;
			case RS_Int4:	pRegisterSetName = "int4"; break;
			case RS_Bool:	pRegisterSetName = "bool"; break;
		};
		if (Obj.ElementCount > 1)
		{
			n_msg(VL_DETAILS, "    Const: %s[%d], %s %d to %d\n",
					Obj.Name.CStr(), Obj.ElementCount, pRegisterSetName, Obj.RegisterStart, Obj.RegisterStart + Obj.ElementRegisterCount * Obj.ElementCount - 1);
		}
		else
		{
			n_msg(VL_DETAILS, "    Const: %s, %s %d to %d\n",
					Obj.Name.CStr(), pRegisterSetName, Obj.RegisterStart, Obj.RegisterStart + Obj.ElementRegisterCount * Obj.ElementCount - 1);
		}
	}

	W.Write<U32>(Meta.Resources.GetCount());
	for (UPTR i = 0; i < Meta.Resources.GetCount(); ++i)
	{
		const CSM30ShaderRsrcMeta& Obj = Meta.Resources[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);

		n_msg(VL_DETAILS, "    Texture: %s, slot %d\n", Obj.Name.CStr(), Obj.Register);
	}

	W.Write<U32>(Meta.Samplers.GetCount());
	for (UPTR i = 0; i < Meta.Samplers.GetCount(); ++i)
	{
		const CSM30ShaderSamplerMeta& Obj = Meta.Samplers[i];
		W.Write(Obj.Name);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);

		if (Obj.RegisterCount > 1)
		{
			n_msg(VL_DETAILS, "    Sampler: %s[%d], slots %d to %d\n", Obj.Name.CStr(), Obj.RegisterCount, Obj.RegisterStart, Obj.RegisterStart + Obj.RegisterCount - 1);
		}
		else
		{
			n_msg(VL_DETAILS, "    Sampler: %s, slot %d\n", Obj.Name.CStr(), Obj.RegisterStart);
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool D3D11SaveShaderMetadata(IO::CBinaryWriter& W, const CD3D11ShaderMeta& Meta)
{
	W.Write<U32>(Meta.Buffers.GetCount());
	for (UPTR i = 0; i < Meta.Buffers.GetCount(); ++i)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		const CD3D11ShaderBufferMeta& Obj = Meta.Buffers[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		W.Write(Obj.ElementSize);
		W.Write(Obj.ElementCount);
		const char* pBufferTypeStr = NULL;
		if (Obj.Register & D3D11Buffer_Texture) pBufferTypeStr = "TBuffer";
		else if (Obj.Register & D3D11Buffer_Structured) pBufferTypeStr = "SBuffer";
		else pBufferTypeStr = "CBuffer";
		n_msg(VL_DETAILS, "    %s: %s, %d slot(s) from %d\n", pBufferTypeStr, Obj.Name.CStr(), 1, Obj.Register);
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

		const char* pTypeString = "<unsupported-type>";
		switch (Obj.Type)
		{
			case D3D11Const_Bool:	pTypeString = "bool"; break;
			case D3D11Const_Int:	pTypeString = "int"; break;
			case D3D11Const_Float:	pTypeString = "float"; break;
		}
		if (Obj.ElementCount > 1)
		{
			n_msg(VL_DETAILS, "      Const: %s %s[%d], type %d, offset %d, size %d\n",
					pTypeString, Obj.Name.CStr(), Obj.ElementCount, Obj.Offset, Obj.ElementSize * Obj.ElementCount);
		}
		else
		{
			n_msg(VL_DETAILS, "      Const: %s %s, type %d, offset %d, size %d\n",
					pTypeString, Obj.Name.CStr(), Obj.Offset, Obj.ElementSize);
		}
	}

	W.Write<U32>(Meta.Resources.GetCount());
	for (UPTR i = 0; i < Meta.Resources.GetCount(); ++i)
	{
		//!!!arrays aren't supported for now!
		const CD3D11ShaderRsrcMeta& Obj = Meta.Resources[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		n_msg(VL_DETAILS, "    Texture: %s, %d slot(s) from %d\n", Obj.Name.CStr(), 1, Obj.Register);
	}

	W.Write<U32>(Meta.Samplers.GetCount());
	for (UPTR i = 0; i < Meta.Samplers.GetCount(); ++i)
	{
		//!!!arrays aren't supported for now!
		const CD3D11ShaderRsrcMeta& Obj = Meta.Samplers[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		n_msg(VL_DETAILS, "    Sampler: %s, %d slot(s) from %d\n", Obj.Name.CStr(), 1, Obj.Register);
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
	}

	R.Read<U32>(Count);
	CSM30ShaderRsrcMeta* pRsrc = Meta.Resources.Reserve(Count, false);
	for (; pRsrc < Meta.Resources.End(); ++pRsrc)
	{
		CSM30ShaderRsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
	}

	R.Read<U32>(Count);
	CSM30ShaderSamplerMeta* pSamp = Meta.Samplers.Reserve(Count, false);
	for (; pSamp < Meta.Samplers.End(); ++pSamp)
	{
		CSM30ShaderSamplerMeta& Obj = *pSamp;
		R.Read(Obj.Name);
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

	U32 Count;

	R.Read<U32>(Count);
	CD3D11ShaderBufferMeta* pBuf = Meta.Buffers.Reserve(Count, false);
	for (; pBuf < Meta.Buffers.End(); ++pBuf)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		CD3D11ShaderBufferMeta& Obj = *pBuf;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		R.Read(Obj.ElementSize);
		R.Read(Obj.ElementCount);
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
	CD3D11ShaderRsrcMeta* pRsrc = Meta.Resources.Reserve(Count, false);
	for (; pRsrc < Meta.Resources.End(); ++pRsrc)
	{
		//!!!arrays aren't supported for now!
		CD3D11ShaderRsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
	}

	R.Read<U32>(Count);
	CD3D11ShaderRsrcMeta* pSamp = Meta.Samplers.Reserve(Count, false);
	for (; pSamp < Meta.Samplers.End(); ++pSamp)
	{
		//!!!arrays aren't supported for now!
		CD3D11ShaderRsrcMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
	}

	OK;
}
//---------------------------------------------------------------------
