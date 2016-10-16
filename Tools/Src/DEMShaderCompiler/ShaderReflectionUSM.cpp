#include "ShaderReflectionUSM.h"

#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <Data/StringUtils.h>
#include <Render/RenderFwd.h>		// For GPU levels enum
#include <D3DCompiler.inl>

extern CString Messages;

bool CUSMBufferMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) FAIL;
	const CUSMBufferMeta& TypedOther = (const CUSMBufferMeta&)Other;
	return Register == TypedOther.Register && Size == TypedOther.Size;
}
//---------------------------------------------------------------------

bool CUSMConstMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) FAIL;
	const CUSMConstMeta& TypedOther = (const CUSMConstMeta&)Other;
	return Type == TypedOther.Type &&
		Offset == TypedOther.Offset &&
		ElementSize == TypedOther.ElementSize &&
		ElementCount == TypedOther.ElementCount;
}
//---------------------------------------------------------------------

bool CUSMRsrcMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) FAIL;
	const CUSMRsrcMeta& TypedOther = (const CUSMRsrcMeta&)Other;
	return Type == TypedOther.Type &&
		RegisterStart == TypedOther.RegisterStart &&
		RegisterCount == TypedOther.RegisterCount;
}
//---------------------------------------------------------------------

bool CUSMSamplerMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) FAIL;
	const CUSMSamplerMeta& TypedOther = (const CUSMSamplerMeta&)Other;
	return RegisterStart == TypedOther.RegisterStart && RegisterCount == TypedOther.RegisterCount;
}
//---------------------------------------------------------------------

// StructCache stores D3D11 type to metadata index mapping, where metadata index is an index in the Out.Structs array
bool CUSMShaderMeta::ProcessStructure(ID3D11ShaderReflectionType* pType, U32 StructSize, CDict<ID3D11ShaderReflectionType*, UPTR>& StructCache)
{
	if (!pType) FAIL;

	// Already processed
	if (StructCache.FindIndex(pType) != INVALID_INDEX) OK;

	D3D11_SHADER_TYPE_DESC D3DTypeDesc;
	pType->GetDesc(&D3DTypeDesc);

	// Has no members
	if (D3DTypeDesc.Members == 0) OK;

	// Add and fill new structure layout metadata
	StructCache.Add(pType, Structs.GetCount());
	CUSMStructMeta& Meta = *Structs.Add();

	CUSMStructMemberMeta* pMemberMeta = Meta.Members.Reserve(D3DTypeDesc.Members);
	for (UINT MemberIdx = 0; MemberIdx < D3DTypeDesc.Members; ++MemberIdx, ++pMemberMeta)
	{
		LPCSTR pMemberName = pType->GetMemberTypeName(MemberIdx);

		ID3D11ShaderReflectionType* pMemberType = pType->GetMemberTypeByIndex(MemberIdx);
		D3D11_SHADER_TYPE_DESC D3DMemberTypeDesc;
		pMemberType->GetDesc(&D3DMemberTypeDesc);

		pMemberMeta->Name = pMemberName;
		pMemberMeta->Offset = D3DMemberTypeDesc.Offset;
		pMemberMeta->Flags = 0;

		U32 MemberSize;
		if (MemberIdx + 1 < D3DTypeDesc.Members)
		{
			ID3D11ShaderReflectionType* pNextMemberType = pType->GetMemberTypeByIndex(MemberIdx + 1);
			D3D11_SHADER_TYPE_DESC D3DNextMemberTypeDesc;
			pNextMemberType->GetDesc(&D3DNextMemberTypeDesc);
			MemberSize = D3DNextMemberTypeDesc.Offset - D3DMemberTypeDesc.Offset;
		}
		else MemberSize = StructSize - D3DMemberTypeDesc.Offset;

		if (D3DMemberTypeDesc.Elements > 1)
		{
			// Arrays
			pMemberMeta->ElementSize = MemberSize / D3DMemberTypeDesc.Elements;
			pMemberMeta->ElementCount = D3DMemberTypeDesc.Elements;
		}
		else
		{
			// Non-arrays and arrays [1]
			pMemberMeta->ElementSize = MemberSize;
			pMemberMeta->ElementCount = 1;
		}

		if (D3DMemberTypeDesc.Class == D3D_SVC_STRUCT)
		{
			pMemberMeta->Type = USMConst_Struct; // D3DTypeDesc.Type is 'void'
			if (!ProcessStructure(pMemberType, pMemberMeta->ElementSize, StructCache)) continue;
			pMemberMeta->StructIndex = StructCache[pMemberType];
		}
		else
		{
			pMemberMeta->StructIndex = (U32)(-1);
			switch (D3DMemberTypeDesc.Type)
			{
				case D3D_SVT_BOOL:	pMemberMeta->Type = USMConst_Bool; break;
				case D3D_SVT_INT:
				case D3D_SVT_UINT:	pMemberMeta->Type = USMConst_Int; break;
				case D3D_SVT_FLOAT:	pMemberMeta->Type = USMConst_Float; break;
				default:
				{
					Messages += "Unsupported constant '";
					Messages += pMemberName;
					Messages += "' type '";
					Messages += StringUtils::FromInt(D3DMemberTypeDesc.Type);
					Messages += "' in a structure '";
					Messages += (D3DTypeDesc.Name ? D3DTypeDesc.Name : "");
					Messages += "'\n";
					FAIL;
				}
			}

			if (D3DTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
				pMemberMeta->Flags |= ShaderConst_ColumnMajor;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::CollectFromBinary(const void* pData, UPTR Size)
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
		case D3D_FEATURE_LEVEL_9_1:		MinFeatureLevel = Render::GPU_Level_D3D9_1; break;
		case D3D_FEATURE_LEVEL_9_2:		MinFeatureLevel = Render::GPU_Level_D3D9_2; break;
		case D3D_FEATURE_LEVEL_9_3:		MinFeatureLevel = Render::GPU_Level_D3D9_3; break;
		case D3D_FEATURE_LEVEL_10_0:	MinFeatureLevel = Render::GPU_Level_D3D10_0; break;
		case D3D_FEATURE_LEVEL_10_1:	MinFeatureLevel = Render::GPU_Level_D3D10_1; break;
		case D3D_FEATURE_LEVEL_11_0:	MinFeatureLevel = Render::GPU_Level_D3D11_0; break;
		//case D3D_FEATURE_LEVEL_11_1:
		default:						MinFeatureLevel = Render::GPU_Level_D3D11_1; break;
	}

	RequiresFlags = pReflector->GetRequiresFlags();

	D3D11_SHADER_DESC D3DDesc;
	if (FAILED(pReflector->GetDesc(&D3DDesc)))
	{
		pReflector->Release();
		FAIL;
	}

	CDict<ID3D11ShaderReflectionType*, UPTR> StructCache;

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
				CUSMRsrcMeta* pMeta = Resources.Add();
				pMeta->Name = RsrcDesc.Name;
				pMeta->RegisterStart = RsrcDesc.BindPoint;
				pMeta->RegisterCount = RsrcDesc.BindCount;

				switch (RsrcDesc.Dimension)
				{
					case D3D_SRV_DIMENSION_TEXTURE1D:			pMeta->Type = USMRsrc_Texture1D; break;
					case D3D_SRV_DIMENSION_TEXTURE1DARRAY:		pMeta->Type = USMRsrc_Texture1DArray; break;
					case D3D_SRV_DIMENSION_TEXTURE2D:			pMeta->Type = USMRsrc_Texture2D; break;
					case D3D_SRV_DIMENSION_TEXTURE2DARRAY:		pMeta->Type = USMRsrc_Texture2DArray; break;
					case D3D_SRV_DIMENSION_TEXTURE2DMS:			pMeta->Type = USMRsrc_Texture2DMS; break;
					case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:	pMeta->Type = USMRsrc_Texture2DMSArray; break;
					case D3D_SRV_DIMENSION_TEXTURE3D:			pMeta->Type = USMRsrc_Texture3D; break;
					case D3D_SRV_DIMENSION_TEXTURECUBE:			pMeta->Type = USMRsrc_TextureCUBE; break;
					case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:	pMeta->Type = USMRsrc_TextureCUBEArray; break;
					default:									pMeta->Type = USMRsrc_Unknown; break;
				}

				break;
			}
			case D3D_SIT_SAMPLER:
			{
				// D3D_SIF_COMPARISON_SAMPLER
				CUSMSamplerMeta* pMeta = Samplers.Add();
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
				if (RsrcDesc.Type == D3D_SIT_TBUFFER) TypeMask = USMBuffer_Texture;
				else if (RsrcDesc.Type == D3D_SIT_STRUCTURED) TypeMask = USMBuffer_Structured;
				else TypeMask = 0;

				CUSMBufferMeta* pMeta = Buffers.Add();
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = (RsrcDesc.BindPoint | TypeMask);
				pMeta->Size = D3DBufDesc.Size;

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

						CUSMConstMeta* pConstMeta = Consts.Add();
						pConstMeta->Name = D3DVarDesc.Name;
						pConstMeta->BufferIndex = Buffers.GetCount() - 1;
						pConstMeta->Offset = D3DVarDesc.StartOffset;
						pConstMeta->Flags = 0;

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

						if (D3DTypeDesc.Class == D3D_SVC_STRUCT)
						{
							pConstMeta->Type = USMConst_Struct; // D3DTypeDesc.Type is 'void'
							if (!ProcessStructure(pVarType, pConstMeta->ElementSize, StructCache)) continue;
							pConstMeta->StructIndex = StructCache[pVarType];
						}
						else
						{
							pConstMeta->StructIndex = (U32)(-1);
							switch (D3DTypeDesc.Type)
							{
								case D3D_SVT_BOOL:	pConstMeta->Type = USMConst_Bool; break;
								case D3D_SVT_INT:
								case D3D_SVT_UINT:	pConstMeta->Type = USMConst_Int; break;
								case D3D_SVT_FLOAT:	pConstMeta->Type = USMConst_Float; break;
								default:
								{
									Messages += "Unsupported constant '";
									Messages += D3DVarDesc.Name;
									Messages += "' type '";
									Messages += StringUtils::FromInt(D3DTypeDesc.Type);
									Messages += "' in USM buffer '";
									Messages += RsrcDesc.Name;
									Messages += "'\n";
									FAIL;
								}
							}

							if (D3DTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
								pConstMeta->Flags |= ShaderConst_ColumnMajor;
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

bool CUSMShaderMeta::Save(IO::CBinaryWriter& W) const
{
	W.Write<U32>(MinFeatureLevel);
	W.Write<U64>(RequiresFlags);

	W.Write<U32>(Buffers.GetCount());
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		const CUSMBufferMeta& Obj = Buffers[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		W.Write(Obj.Size);
		//W.Write(Obj.ElementCount);
	}

	W.Write<U32>(Structs.GetCount());
	for (UPTR i = 0; i < Structs.GetCount(); ++i)
	{
		CUSMStructMeta& Obj = Structs[i];

		W.Write<U32>(Obj.Members.GetCount());
		for (UPTR j = 0; j < Obj.Members.GetCount(); ++j)
		{
			CUSMStructMemberMeta& Member = Obj.Members[j];
			W.Write(Member.Name);
			W.Write(Member.StructIndex);
			W.Write<U8>(Member.Type);
			W.Write(Member.Offset);
			W.Write(Member.ElementSize);
			W.Write(Member.ElementCount);
			W.Write(Member.Flags);
		}
	}

	W.Write<U32>(Consts.GetCount());
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const CUSMConstMeta& Obj = Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write(Obj.StructIndex);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.Offset);
		W.Write(Obj.ElementSize);
		W.Write(Obj.ElementCount);
		W.Write(Obj.Flags);
	}

	W.Write<U32>(Resources.GetCount());
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		const CUSMRsrcMeta& Obj = Resources[i];
		W.Write(Obj.Name);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	W.Write<U32>(Samplers.GetCount());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		const CUSMSamplerMeta& Obj = Samplers[i];
		W.Write(Obj.Name);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::Load(IO::CBinaryReader& R)
{
	Buffers.Clear();
	Consts.Clear();
	Resources.Clear();
	Samplers.Clear();

	R.Read<U32>(MinFeatureLevel);
	R.Read<U64>(RequiresFlags);

	U32 Count;

	R.Read<U32>(Count);
	CUSMBufferMeta* pBuf = Buffers.Reserve(Count, false);
	for (; pBuf < Buffers.End(); ++pBuf)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		CUSMBufferMeta& Obj = *pBuf;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		R.Read(Obj.Size);
		///R.Read(Obj.ElementCount);
	}

	R.Read<U32>(Count);
	CUSMStructMeta* pStruct = Structs.Reserve(Count, false);
	for (; pStruct < Structs.End(); ++pStruct)
	{
		CUSMStructMeta& Obj = *pStruct;

		R.Read<U32>(Count);
		CUSMStructMemberMeta* pMember = Obj.Members.Reserve(Count, false);
		for (; pMember < Obj.Members.End(); ++pMember)
		{
			CUSMStructMemberMeta& Member = *pMember;
			R.Read(Member.Name);
			R.Read(Member.StructIndex);

			U8 Type;
			R.Read<U8>(Type);
			Member.Type = (EUSMConstType)Type;

			R.Read(Member.Offset);
			R.Read(Member.ElementSize);
			R.Read(Member.ElementCount);
			R.Read(Member.Flags);
		}
	}

	R.Read<U32>(Count);
	CUSMConstMeta* pConst = Consts.Reserve(Count, false);
	for (; pConst < Consts.End(); ++pConst)
	{
		CUSMConstMeta& Obj = *pConst;
		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);
		R.Read(Obj.StructIndex);

		U8 Type;
		R.Read<U8>(Type);
		Obj.Type = (EUSMConstType)Type;

		R.Read(Obj.Offset);
		R.Read(Obj.ElementSize);
		R.Read(Obj.ElementCount);
		R.Read(Obj.Flags);
	}

	R.Read<U32>(Count);
	CUSMRsrcMeta* pRsrc = Resources.Reserve(Count, false);
	for (; pRsrc < Resources.End(); ++pRsrc)
	{
		CUSMRsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);

		U8 Type;
		R.Read<U8>(Type);
		Obj.Type = (EUSMResourceType)Type;

		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	R.Read<U32>(Count);
	CUSMSamplerMeta* pSamp = Samplers.Reserve(Count, false);
	for (; pSamp < Samplers.End(); ++pSamp)
	{
		CUSMSamplerMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

UPTR CUSMShaderMeta::GetParamCount(EShaderParamClass Class) const
{
	switch (Class)
	{
		case ShaderParam_Const:		return Consts.GetCount();
		case ShaderParam_Resource:	return Resources.GetCount();
		case ShaderParam_Sampler:	return Samplers.GetCount();
		default:					return 0;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CUSMShaderMeta::GetParamObject(EShaderParamClass Class, UPTR Index)
{
	switch (Class)
	{
		case ShaderParam_Const:		return &Consts[Index];
		case ShaderParam_Resource:	return &Resources[Index];
		case ShaderParam_Sampler:	return &Samplers[Index];
		default:					return NULL;
	}
}
//---------------------------------------------------------------------

UPTR CUSMShaderMeta::AddParamObject(EShaderParamClass Class, const CMetadataObject* pMetaObject)
{
	if (!pMetaObject || pMetaObject->GetShaderModel() != GetShaderModel() || pMetaObject->GetClass() != Class) return (UPTR)(INVALID_INDEX);

	switch (Class)
	{
		case ShaderParam_Const:
		{
			Consts.Add(*(const CUSMConstMeta*)pMetaObject);
			return Consts.GetCount() - 1;
		}
		case ShaderParam_Resource:
		{
			Resources.Add(*(const CUSMRsrcMeta*)pMetaObject);
			return Resources.GetCount() - 1;
		}
		case ShaderParam_Sampler:
		{
			Samplers.Add(*(const CUSMSamplerMeta*)pMetaObject);
			return Samplers.GetCount() - 1;
		}
		default:	return (UPTR)(INVALID_INDEX);
	}
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::FindParamObjectByName(EShaderParamClass Class, const char* pName, UPTR& OutIndex) const
{
	switch (Class)
	{
		case ShaderParam_Const:
		{
			UPTR Idx = 0;
			for (; Idx < Consts.GetCount(); ++ Idx)
				if (Consts[Idx].Name == pName) break;
			if (Idx == Consts.GetCount()) FAIL;
			OutIndex = Idx;
			OK;
		}
		case ShaderParam_Resource:
		{
			UPTR Idx = 0;
			for (; Idx < Resources.GetCount(); ++ Idx)
				if (Resources[Idx].Name == pName) break;
			if (Idx == Resources.GetCount()) FAIL;
			OutIndex = Idx;
			OK;
		}
		case ShaderParam_Sampler:
		{
			UPTR Idx = 0;
			for (; Idx < Samplers.GetCount(); ++ Idx)
				if (Samplers[Idx].Name == pName) break;
			if (Idx == Samplers.GetCount()) FAIL;
			OutIndex = Idx;
			OK;
		}
		default:					FAIL;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CUSMShaderMeta::GetContainingConstantBuffer(CMetadataObject* pMetaObject)
{
	if (!pMetaObject || pMetaObject->GetClass() != ShaderParam_Const || pMetaObject->GetShaderModel() != GetShaderModel()) return NULL;
	return &Buffers[((CUSMConstMeta*)pMetaObject)->BufferIndex];
}
//---------------------------------------------------------------------
