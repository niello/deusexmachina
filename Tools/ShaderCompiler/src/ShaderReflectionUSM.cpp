#include "ShaderReflectionUSM.h"

#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <Data/StringUtils.h>
#include <Render/RenderFwd.h>		// For GPU levels enum
#include <D3DCompiler.inl>

extern std::string Messages;

bool CUSMBufferMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) return false;
	const CUSMBufferMeta& TypedOther = (const CUSMBufferMeta&)Other;
	return Register == TypedOther.Register && Size == TypedOther.Size;
}
//---------------------------------------------------------------------

bool CUSMConstMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) return false;
	const CUSMConstMeta& TypedOther = (const CUSMConstMeta&)Other;
	return Type == TypedOther.Type &&
		Offset == TypedOther.Offset &&
		ElementSize == TypedOther.ElementSize &&
		ElementCount == TypedOther.ElementCount;
}
//---------------------------------------------------------------------

bool CUSMRsrcMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) return false;
	const CUSMRsrcMeta& TypedOther = (const CUSMRsrcMeta&)Other;
	return Type == TypedOther.Type &&
		RegisterStart == TypedOther.RegisterStart &&
		RegisterCount == TypedOther.RegisterCount;
}
//---------------------------------------------------------------------

bool CUSMSamplerMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) return false;
	const CUSMSamplerMeta& TypedOther = (const CUSMSamplerMeta&)Other;
	return RegisterStart == TypedOther.RegisterStart && RegisterCount == TypedOther.RegisterCount;
}
//---------------------------------------------------------------------

// StructCache stores D3D11 type to metadata index mapping, where metadata index is an index in the Out.Structs array
bool CUSMShaderMeta::ProcessStructure(ID3D11ShaderReflectionType* pType, uint32_t StructSize, std::map<ID3D11ShaderReflectionType*, size_t>& StructCache)
{
	if (!pType) return false;

	// Already processed
	if (StructCache.FindIndex(pType) != INVALID_INDEX) return true;

	D3D11_SHADER_TYPE_DESC D3DTypeDesc;
	pType->GetDesc(&D3DTypeDesc);

	// Has no members
	if (D3DTypeDesc.Members == 0) return true;

	// Add and fill new structure layout metadata
	StructCache.Add(pType, Structs.size());
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

		uint32_t MemberSize;
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
			pMemberMeta->StructIndex = (uint32_t)(-1);
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
					return false;
				}
			}

			pMemberMeta->Columns = D3DMemberTypeDesc.Columns;
			pMemberMeta->Rows = D3DMemberTypeDesc.Rows;

			if (D3DMemberTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
				pMemberMeta->Flags |= ShaderConst_ColumnMajor;
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::CollectFromBinary(const void* pData, size_t Size)
{
	ID3D11ShaderReflection* pReflector = nullptr;

	if (FAILED(D3D11Reflect(pData, Size, &pReflector))) return false;

	D3D_FEATURE_LEVEL D3DFeatureLevel;
	if (FAILED(pReflector->GetMinFeatureLevel(&D3DFeatureLevel)))
	{
		pReflector->Release();
		return false;
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
		return false;
	}

	std::map<ID3D11ShaderReflectionType*, size_t> StructCache;

	for (UINT RsrcIdx = 0; RsrcIdx < D3DDesc.BoundResources; ++RsrcIdx)
	{
		D3D11_SHADER_INPUT_BIND_DESC RsrcDesc;
		if (FAILED(pReflector->GetResourceBindingDesc(RsrcIdx, &RsrcDesc)))
		{
			pReflector->Release();
			return false;
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

				ID3D11ShaderReflectionConstantBuffer* pCB = nullptr;
				D3D11_SHADER_BUFFER_DESC D3DBufDesc;
				for (size_t BufIdx = 0; BufIdx < D3DDesc.ConstantBuffers; ++BufIdx)
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
						pConstMeta->BufferIndex = Buffers.size() - 1;
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
							pConstMeta->StructIndex = (uint32_t)(-1);
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
									return false;
								}
							}

							pConstMeta->Columns = D3DTypeDesc.Columns;
							pConstMeta->Rows = D3DTypeDesc.Rows;

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

	return true;
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::Save(IO::CBinaryWriter& W) const
{
	W.Write<uint32_t>(MinFeatureLevel);
	W.Write<uint64_t>(RequiresFlags);

	W.Write<uint32_t>(Buffers.size());
	for (size_t i = 0; i < Buffers.size(); ++i)
	{
		const CUSMBufferMeta& Obj = Buffers[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		W.Write(Obj.Size);
		//W.Write(Obj.ElementCount);
	}

	W.Write<uint32_t>(Structs.size());
	for (size_t i = 0; i < Structs.size(); ++i)
	{
		CUSMStructMeta& Obj = Structs[i];

		W.Write<uint32_t>(Obj.Members.size());
		for (size_t j = 0; j < Obj.Members.size(); ++j)
		{
			CUSMStructMemberMeta& Member = Obj.Members[j];
			W.Write(Member.Name);
			W.Write(Member.StructIndex);
			W.Write<uint8_t>(Member.Type);
			W.Write(Member.Offset);
			W.Write(Member.ElementSize);
			W.Write(Member.ElementCount);
			W.Write(Member.Columns);
			W.Write(Member.Rows);
			W.Write(Member.Flags);
		}
	}

	W.Write<uint32_t>(Consts.size());
	for (size_t i = 0; i < Consts.size(); ++i)
	{
		const CUSMConstMeta& Obj = Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write(Obj.StructIndex);
		W.Write<uint8_t>(Obj.Type);
		W.Write(Obj.Offset);
		W.Write(Obj.ElementSize);
		W.Write(Obj.ElementCount);
		W.Write(Obj.Columns);
		W.Write(Obj.Rows);
		W.Write(Obj.Flags);
	}

	W.Write<uint32_t>(Resources.size());
	for (size_t i = 0; i < Resources.size(); ++i)
	{
		const CUSMRsrcMeta& Obj = Resources[i];
		W.Write(Obj.Name);
		W.Write<uint8_t>(Obj.Type);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	W.Write<uint32_t>(Samplers.size());
	for (size_t i = 0; i < Samplers.size(); ++i)
	{
		const CUSMSamplerMeta& Obj = Samplers[i];
		W.Write(Obj.Name);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	return true;
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::Load(IO::CBinaryReader& R)
{
	Buffers.Clear();
	Consts.Clear();
	Resources.Clear();
	Samplers.Clear();

	R.Read<uint32_t>(MinFeatureLevel);
	R.Read<uint64_t>(RequiresFlags);

	uint32_t Count;

	R.Read<uint32_t>(Count);
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

	R.Read<uint32_t>(Count);
	CUSMStructMeta* pStruct = Structs.Reserve(Count, false);
	for (; pStruct < Structs.End(); ++pStruct)
	{
		CUSMStructMeta& Obj = *pStruct;

		R.Read<uint32_t>(Count);
		CUSMStructMemberMeta* pMember = Obj.Members.Reserve(Count, false);
		for (; pMember < Obj.Members.End(); ++pMember)
		{
			CUSMStructMemberMeta& Member = *pMember;
			R.Read(Member.Name);
			R.Read(Member.StructIndex);

			uint8_t Type;
			R.Read<uint8_t>(Type);
			Member.Type = (EUSMConstType)Type;

			R.Read(Member.Offset);
			R.Read(Member.ElementSize);
			R.Read(Member.ElementCount);
			R.Read(Member.Columns);
			R.Read(Member.Rows);
			R.Read(Member.Flags);
		}
	}

	R.Read<uint32_t>(Count);
	CUSMConstMeta* pConst = Consts.Reserve(Count, false);
	for (; pConst < Consts.End(); ++pConst)
	{
		CUSMConstMeta& Obj = *pConst;
		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);
		R.Read(Obj.StructIndex);

		uint8_t Type;
		R.Read<uint8_t>(Type);
		Obj.Type = (EUSMConstType)Type;

		R.Read(Obj.Offset);
		R.Read(Obj.ElementSize);
		R.Read(Obj.ElementCount);
		R.Read(Obj.Columns);
		R.Read(Obj.Rows);
		R.Read(Obj.Flags);
	}

	R.Read<uint32_t>(Count);
	CUSMRsrcMeta* pRsrc = Resources.Reserve(Count, false);
	for (; pRsrc < Resources.End(); ++pRsrc)
	{
		CUSMRsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);

		uint8_t Type;
		R.Read<uint8_t>(Type);
		Obj.Type = (EUSMResourceType)Type;

		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	R.Read<uint32_t>(Count);
	CUSMSamplerMeta* pSamp = Samplers.Reserve(Count, false);
	for (; pSamp < Samplers.End(); ++pSamp)
	{
		CUSMSamplerMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	return true;
}
//---------------------------------------------------------------------

size_t CUSMShaderMeta::GetParamCount(EShaderParamClass Class) const
{
	switch (Class)
	{
		case ShaderParam_Const:		return Consts.size();
		case ShaderParam_Resource:	return Resources.size();
		case ShaderParam_Sampler:	return Samplers.size();
		default:					return 0;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CUSMShaderMeta::GetParamObject(EShaderParamClass Class, size_t Index)
{
	switch (Class)
	{
		case ShaderParam_Const:		return &Consts[Index];
		case ShaderParam_Resource:	return &Resources[Index];
		case ShaderParam_Sampler:	return &Samplers[Index];
		default:					return nullptr;
	}
}
//---------------------------------------------------------------------

size_t CUSMShaderMeta::AddParamObject(EShaderParamClass Class, const CMetadataObject* pMetaObject)
{
	if (!pMetaObject || pMetaObject->GetShaderModel() != GetShaderModel() || pMetaObject->GetClass() != Class) return (size_t)(INVALID_INDEX);

	switch (Class)
	{
		case ShaderParam_Const:
		{
			Consts.Add(*(const CUSMConstMeta*)pMetaObject);
			return Consts.size() - 1;
		}
		case ShaderParam_Resource:
		{
			Resources.Add(*(const CUSMRsrcMeta*)pMetaObject);
			return Resources.size() - 1;
		}
		case ShaderParam_Sampler:
		{
			Samplers.Add(*(const CUSMSamplerMeta*)pMetaObject);
			return Samplers.size() - 1;
		}
		default:	return (size_t)(INVALID_INDEX);
	}
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::FindParamObjectByName(EShaderParamClass Class, const char* pName, size_t& OutIndex) const
{
	switch (Class)
	{
		case ShaderParam_Const:
		{
			size_t Idx = 0;
			for (; Idx < Consts.size(); ++ Idx)
				if (Consts[Idx].Name == pName) break;
			if (Idx == Consts.size()) return false;
			OutIndex = Idx;
			return true;
		}
		case ShaderParam_Resource:
		{
			size_t Idx = 0;
			for (; Idx < Resources.size(); ++ Idx)
				if (Resources[Idx].Name == pName) break;
			if (Idx == Resources.size()) return false;
			OutIndex = Idx;
			return true;
		}
		case ShaderParam_Sampler:
		{
			size_t Idx = 0;
			for (; Idx < Samplers.size(); ++ Idx)
				if (Samplers[Idx].Name == pName) break;
			if (Idx == Samplers.size()) return false;
			OutIndex = Idx;
			return true;
		}
		default:					return false;
	}
}
//---------------------------------------------------------------------

size_t CUSMShaderMeta::AddOrMergeBuffer(const CMetadataObject* pMetaBuffer)
{
	if (!pMetaBuffer || pMetaBuffer->GetShaderModel() != GetShaderModel()) return (size_t)(INVALID_INDEX);

	const CUSMBufferMeta* pUSMBuffer = (const CUSMBufferMeta*)pMetaBuffer;
	size_t Idx = 0;
	for (; Idx < Buffers.size(); ++ Idx)
		if (Buffers[Idx].Register == pUSMBuffer->Register) break;
	if (Idx == Buffers.size())
	{
		Buffers.Add(*pUSMBuffer);
		return Buffers.size() - 1;
	}
	else
	{
		// Use a bigger of conflicting buffers
		if (Buffers[Idx].Size < pUSMBuffer->Size)
			Buffers[Idx] = *pUSMBuffer;
		return Idx;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CUSMShaderMeta::GetContainingConstantBuffer(const CMetadataObject* pMetaObject) const
{
	if (!pMetaObject || pMetaObject->GetClass() != ShaderParam_Const || pMetaObject->GetShaderModel() != GetShaderModel()) return nullptr;
	return &Buffers[((CUSMConstMeta*)pMetaObject)->BufferIndex];
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::SetContainingConstantBuffer(size_t ConstIdx, size_t BufferIdx)
{
	if (ConstIdx >= Consts.size()) return false;
	Consts[ConstIdx].BufferIndex = BufferIdx;
	return true;
}
//---------------------------------------------------------------------

uint32_t CUSMShaderMeta::AddStructure(const CShaderMetadata& SourceMeta, uint64_t StructKey, std::map<uint64_t, uint32_t>& StructIndexMapping)
{
	ptrdiff_t StructIdxIdx = StructIndexMapping.FindIndex(StructKey);
	if (StructIdxIdx != INVALID_INDEX) return StructIndexMapping.ValueAt(StructIdxIdx);

	const uint32_t StructIndex = (uint32_t)(StructKey & 0xffffffff);
	const CUSMStructMeta& StructMeta = ((const CUSMShaderMeta&)SourceMeta).Structs[StructIndex];
	Structs.Add(StructMeta);
	StructIndexMapping.Add(StructKey, Structs.size() - 1);

	for (size_t i = 0; i < StructMeta.Members.size(); ++i)
	{
		const CUSMStructMemberMeta& MemberMeta = StructMeta.Members[i];
		if (MemberMeta.StructIndex == (uint32_t)(-1)) continue;

		const uint64_t MemberKey = (StructKey & 0xffffffff00000000) | ((uint64_t)MemberMeta.StructIndex);
		AddStructure(SourceMeta, MemberKey, StructIndexMapping);
	}

	StructIdxIdx = StructIndexMapping.FindIndex(StructKey);
	n_assert(StructIdxIdx != INVALID_INDEX);
	return StructIndexMapping.ValueAt(StructIdxIdx);
}
//---------------------------------------------------------------------

uint32_t CUSMShaderMeta::GetStructureIndex(size_t ConstIdx) const
{
	if (ConstIdx >= Consts.size()) return (uint32_t)(-1);
	return Consts[ConstIdx].StructIndex;
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::SetStructureIndex(size_t ConstIdx, uint32_t StructIdx)
{
	if (ConstIdx >= Consts.size()) return false;
	Consts[ConstIdx].StructIndex = StructIdx;
	return true;
}
//---------------------------------------------------------------------
