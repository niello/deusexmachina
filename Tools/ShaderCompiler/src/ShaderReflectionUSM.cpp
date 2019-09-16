#include "ShaderReflectionUSM.h"

#include <Utils.h>
#include <cassert>
#include <D3DCompiler.inl>

#undef min
#undef max

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
	if (StructCache.find(pType) != StructCache.cend()) return true;

	D3D11_SHADER_TYPE_DESC D3DTypeDesc;
	pType->GetDesc(&D3DTypeDesc);

	// Has no members
	if (D3DTypeDesc.Members == 0) return true;

	// Add and fill new structure layout metadata
	StructCache.emplace(pType, Structs.size());
	CUSMStructMeta Meta;

	Meta.Members.reserve(D3DTypeDesc.Members);
	for (UINT MemberIdx = 0; MemberIdx < D3DTypeDesc.Members; ++MemberIdx)
	{
		LPCSTR pMemberName = pType->GetMemberTypeName(MemberIdx);

		ID3D11ShaderReflectionType* pMemberType = pType->GetMemberTypeByIndex(MemberIdx);
		D3D11_SHADER_TYPE_DESC D3DMemberTypeDesc;
		pMemberType->GetDesc(&D3DMemberTypeDesc);

		CUSMStructMemberMeta MemberMeta;
		MemberMeta.Name = pMemberName;
		MemberMeta.Offset = D3DMemberTypeDesc.Offset;
		MemberMeta.Flags = 0;

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
			MemberMeta.ElementSize = MemberSize / D3DMemberTypeDesc.Elements;
			MemberMeta.ElementCount = D3DMemberTypeDesc.Elements;
		}
		else
		{
			// Non-arrays and arrays [1]
			MemberMeta.ElementSize = MemberSize;
			MemberMeta.ElementCount = 1;
		}

		if (D3DMemberTypeDesc.Class == D3D_SVC_STRUCT)
		{
			MemberMeta.Type = USMConst_Struct; // D3DTypeDesc.Type is 'void'
			if (!ProcessStructure(pMemberType, MemberMeta.ElementSize, StructCache)) continue;
			MemberMeta.StructIndex = StructCache[pMemberType];
		}
		else
		{
			MemberMeta.StructIndex = (uint32_t)(-1);
			switch (D3DMemberTypeDesc.Type)
			{
				case D3D_SVT_BOOL:	MemberMeta.Type = USMConst_Bool; break;
				case D3D_SVT_INT:
				case D3D_SVT_UINT:	MemberMeta.Type = USMConst_Int; break;
				case D3D_SVT_FLOAT:	MemberMeta.Type = USMConst_Float; break;
				default:
				{
					Messages += "Unsupported constant '";
					Messages += pMemberName;
					Messages += "' type '";
					Messages += std::to_string(D3DMemberTypeDesc.Type);
					Messages += "' in a structure '";
					Messages += (D3DTypeDesc.Name ? D3DTypeDesc.Name : "");
					Messages += "'\n";
					return false;
				}
			}

			MemberMeta.Columns = D3DMemberTypeDesc.Columns;
			MemberMeta.Rows = D3DMemberTypeDesc.Rows;

			if (D3DMemberTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
				MemberMeta.Flags |= ShaderConst_ColumnMajor;
		}

		Meta.Members.push_back(std::move(MemberMeta));
	}

	Structs.push_back(std::move(Meta));

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
		case D3D_FEATURE_LEVEL_9_1:		MinFeatureLevel = GPU_Level_D3D9_1; break;
		case D3D_FEATURE_LEVEL_9_2:		MinFeatureLevel = GPU_Level_D3D9_2; break;
		case D3D_FEATURE_LEVEL_9_3:		MinFeatureLevel = GPU_Level_D3D9_3; break;
		case D3D_FEATURE_LEVEL_10_0:	MinFeatureLevel = GPU_Level_D3D10_0; break;
		case D3D_FEATURE_LEVEL_10_1:	MinFeatureLevel = GPU_Level_D3D10_1; break;
		case D3D_FEATURE_LEVEL_11_0:	MinFeatureLevel = GPU_Level_D3D11_0; break;
		//case D3D_FEATURE_LEVEL_11_1:
		default:						MinFeatureLevel = GPU_Level_D3D11_1; break;
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
				CUSMRsrcMeta Meta;
				Meta.Name = RsrcDesc.Name;
				Meta.RegisterStart = RsrcDesc.BindPoint;
				Meta.RegisterCount = RsrcDesc.BindCount;

				switch (RsrcDesc.Dimension)
				{
					case D3D_SRV_DIMENSION_TEXTURE1D:			Meta.Type = USMRsrc_Texture1D; break;
					case D3D_SRV_DIMENSION_TEXTURE1DARRAY:		Meta.Type = USMRsrc_Texture1DArray; break;
					case D3D_SRV_DIMENSION_TEXTURE2D:			Meta.Type = USMRsrc_Texture2D; break;
					case D3D_SRV_DIMENSION_TEXTURE2DARRAY:		Meta.Type = USMRsrc_Texture2DArray; break;
					case D3D_SRV_DIMENSION_TEXTURE2DMS:			Meta.Type = USMRsrc_Texture2DMS; break;
					case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:	Meta.Type = USMRsrc_Texture2DMSArray; break;
					case D3D_SRV_DIMENSION_TEXTURE3D:			Meta.Type = USMRsrc_Texture3D; break;
					case D3D_SRV_DIMENSION_TEXTURECUBE:			Meta.Type = USMRsrc_TextureCUBE; break;
					case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:	Meta.Type = USMRsrc_TextureCUBEArray; break;
					default:									Meta.Type = USMRsrc_Unknown; break;
				}

				Resources.push_back(std::move(Meta));

				break;
			}
			case D3D_SIT_SAMPLER:
			{
				// D3D_SIF_COMPARISON_SAMPLER
				CUSMSamplerMeta Meta;
				Meta.Name = RsrcDesc.Name;
				Meta.RegisterStart = RsrcDesc.BindPoint;
				Meta.RegisterCount = RsrcDesc.BindCount;
				Samplers.push_back(std::move(Meta));
				break;
			}
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER:
			case D3D_SIT_STRUCTURED:
			{
				// Can't violate this condition (can't define cbuffer / tbuffer array)
				assert(RsrcDesc.BindCount == 1);

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

				CUSMBufferMeta Meta;
				Meta.Name = RsrcDesc.Name;
				Meta.Register = (RsrcDesc.BindPoint | TypeMask);
				Meta.Size = D3DBufDesc.Size;
				Buffers.push_back(std::move(Meta));

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

						CUSMConstMeta ConstMeta;
						ConstMeta.Name = D3DVarDesc.Name;
						ConstMeta.BufferIndex = Buffers.size() - 1;
						ConstMeta.Offset = D3DVarDesc.StartOffset;
						ConstMeta.Flags = 0;

						if (D3DTypeDesc.Elements > 1)
						{
							// Arrays
							ConstMeta.ElementSize = D3DVarDesc.Size / D3DTypeDesc.Elements;
							ConstMeta.ElementCount = D3DTypeDesc.Elements;
						}
						else
						{
							// Non-arrays and arrays [1]
							ConstMeta.ElementSize = D3DVarDesc.Size;
							ConstMeta.ElementCount = 1;
						}

						if (D3DTypeDesc.Class == D3D_SVC_STRUCT)
						{
							ConstMeta.Type = USMConst_Struct; // D3DTypeDesc.Type is 'void'
							if (!ProcessStructure(pVarType, ConstMeta.ElementSize, StructCache)) continue;
							ConstMeta.StructIndex = StructCache[pVarType];
						}
						else
						{
							ConstMeta.StructIndex = (uint32_t)(-1);
							switch (D3DTypeDesc.Type)
							{
								case D3D_SVT_BOOL:	ConstMeta.Type = USMConst_Bool; break;
								case D3D_SVT_INT:
								case D3D_SVT_UINT:	ConstMeta.Type = USMConst_Int; break;
								case D3D_SVT_FLOAT:	ConstMeta.Type = USMConst_Float; break;
								default:
								{
									Messages += "Unsupported constant '";
									Messages += D3DVarDesc.Name;
									Messages += "' type '";
									Messages += std::to_string(D3DTypeDesc.Type);
									Messages += "' in USM buffer '";
									Messages += RsrcDesc.Name;
									Messages += "'\n";
									return false;
								}
							}

							ConstMeta.Columns = D3DTypeDesc.Columns;
							ConstMeta.Rows = D3DTypeDesc.Rows;

							if (D3DTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
								ConstMeta.Flags |= ShaderConst_ColumnMajor;

							Consts.push_back(std::move(ConstMeta));
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

bool CUSMShaderMeta::Save(std::ostream& Stream) const
{
	WriteStream<uint32_t>(Stream, MinFeatureLevel);
	WriteStream<uint64_t>(Stream, RequiresFlags);

	WriteStream<uint32_t>(Stream, Buffers.size());
	for (size_t i = 0; i < Buffers.size(); ++i)
	{
		const CUSMBufferMeta& Obj = Buffers[i];
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.Register);
		WriteStream(Stream, Obj.Size);
		//WriteStream(Stream, Obj.ElementCount);
	}

	WriteStream<uint32_t>(Stream, Structs.size());
	for (size_t i = 0; i < Structs.size(); ++i)
	{
		const CUSMStructMeta& Obj = Structs[i];

		WriteStream<uint32_t>(Stream, Obj.Members.size());
		for (size_t j = 0; j < Obj.Members.size(); ++j)
		{
			const CUSMStructMemberMeta& Member = Obj.Members[j];
			WriteStream(Stream, Member.Name);
			WriteStream(Stream, Member.StructIndex);
			WriteStream<uint8_t>(Stream, Member.Type);
			WriteStream(Stream, Member.Offset);
			WriteStream(Stream, Member.ElementSize);
			WriteStream(Stream, Member.ElementCount);
			WriteStream(Stream, Member.Columns);
			WriteStream(Stream, Member.Rows);
			WriteStream(Stream, Member.Flags);
		}
	}

	WriteStream<uint32_t>(Stream, Consts.size());
	for (size_t i = 0; i < Consts.size(); ++i)
	{
		const CUSMConstMeta& Obj = Consts[i];
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.BufferIndex);
		WriteStream(Stream, Obj.StructIndex);
		WriteStream<uint8_t>(Stream, Obj.Type);
		WriteStream(Stream, Obj.Offset);
		WriteStream(Stream, Obj.ElementSize);
		WriteStream(Stream, Obj.ElementCount);
		WriteStream(Stream, Obj.Columns);
		WriteStream(Stream, Obj.Rows);
		WriteStream(Stream, Obj.Flags);
	}

	WriteStream<uint32_t>(Stream, Resources.size());
	for (size_t i = 0; i < Resources.size(); ++i)
	{
		const CUSMRsrcMeta& Obj = Resources[i];
		WriteStream(Stream, Obj.Name);
		WriteStream<uint8_t>(Stream, Obj.Type);
		WriteStream(Stream, Obj.RegisterStart);
		WriteStream(Stream, Obj.RegisterCount);
	}

	WriteStream<uint32_t>(Stream, Samplers.size());
	for (size_t i = 0; i < Samplers.size(); ++i)
	{
		const CUSMSamplerMeta& Obj = Samplers[i];
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.RegisterStart);
		WriteStream(Stream, Obj.RegisterCount);
	}

	return true;
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::Load(std::istream& Stream)
{
	Buffers.clear();
	Consts.clear();
	Resources.clear();
	Samplers.clear();

	ReadStream<uint32_t>(Stream, MinFeatureLevel);
	ReadStream<uint64_t>(Stream, RequiresFlags);

	uint32_t Count;

	ReadStream<uint32_t>(Stream, Count);
	Buffers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		CUSMBufferMeta Obj;
		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.Register);
		ReadStream(Stream, Obj.Size);
		///ReadStream(Obj.ElementCount);
		Buffers.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Structs.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMStructMeta Obj;

		uint32_t MemberCount;
		ReadStream<uint32_t>(Stream, MemberCount);
		Obj.Members.reserve(MemberCount);
		for (uint32_t j = 0; j < MemberCount; ++j)
		{
			CUSMStructMemberMeta Member;
			ReadStream(Stream, Member.Name);
			ReadStream(Stream, Member.StructIndex);

			uint8_t Type;
			ReadStream<uint8_t>(Stream, Type);
			Member.Type = (EUSMConstType)Type;

			ReadStream(Stream, Member.Offset);
			ReadStream(Stream, Member.ElementSize);
			ReadStream(Stream, Member.ElementCount);
			ReadStream(Stream, Member.Columns);
			ReadStream(Stream, Member.Rows);
			ReadStream(Stream, Member.Flags);

			Obj.Members.push_back(std::move(Member));
		}

		Structs.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Consts.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMConstMeta Obj;
		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.BufferIndex);
		ReadStream(Stream, Obj.StructIndex);

		uint8_t Type;
		ReadStream<uint8_t>(Stream, Type);
		Obj.Type = (EUSMConstType)Type;

		ReadStream(Stream, Obj.Offset);
		ReadStream(Stream, Obj.ElementSize);
		ReadStream(Stream, Obj.ElementCount);
		ReadStream(Stream, Obj.Columns);
		ReadStream(Stream, Obj.Rows);
		ReadStream(Stream, Obj.Flags);

		Consts.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Resources.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMRsrcMeta Obj;
		ReadStream(Stream, Obj.Name);

		uint8_t Type;
		ReadStream<uint8_t>(Stream, Type);
		Obj.Type = (EUSMResourceType)Type;

		ReadStream(Stream, Obj.RegisterStart);
		ReadStream(Stream, Obj.RegisterCount);

		Resources.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Samplers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMSamplerMeta Obj;
		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.RegisterStart);
		ReadStream(Stream, Obj.RegisterCount);
		Samplers.push_back(std::move(Obj));
	}

	return true;
}
//---------------------------------------------------------------------
