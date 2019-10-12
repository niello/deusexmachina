#include <ShaderReflection.h>
#include <ShaderMeta/USMShaderMeta.h>
#include <LogDelegate.h>
#include <cassert>
#include <map>
#define WIN32_LEAN_AND_MEAN
#include <d3dcompiler.h>

// StructCache stores D3D11 type to metadata index mapping, where metadata index is an index in the Out.Structs array
static bool ProcessStructure(CUSMShaderMeta& Meta, ID3D11ShaderReflectionType* pType, uint32_t StructSize,
	std::map<ID3D11ShaderReflectionType*, size_t>& StructCache, DEMShaderCompiler::ILogDelegate* pLog)
{
	if (!pType) return false;

	// Already processed
	if (StructCache.find(pType) != StructCache.cend()) return true;

	D3D11_SHADER_TYPE_DESC D3DTypeDesc;
	pType->GetDesc(&D3DTypeDesc);

	// Has no members
	if (D3DTypeDesc.Members == 0) return true;

	// Add and fill new structure layout metadata
	StructCache.emplace(pType, Meta.Structs.size());
	CUSMStructMeta StructMeta;
	StructMeta.Name = D3DTypeDesc.Name ? D3DTypeDesc.Name : std::string();

	StructMeta.Members.reserve(D3DTypeDesc.Members);
	for (UINT MemberIdx = 0; MemberIdx < D3DTypeDesc.Members; ++MemberIdx)
	{
		LPCSTR pMemberName = pType->GetMemberTypeName(MemberIdx);

		ID3D11ShaderReflectionType* pMemberType = pType->GetMemberTypeByIndex(MemberIdx);
		D3D11_SHADER_TYPE_DESC D3DMemberTypeDesc;
		pMemberType->GetDesc(&D3DMemberTypeDesc);

		CUSMConstMetaBase MemberMeta;
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
			if (!ProcessStructure(Meta, pMemberType, MemberMeta.ElementSize, StructCache, pLog)) continue;
			MemberMeta.StructIndex = StructCache[pMemberType];
		}
		else
		{
			MemberMeta.StructIndex = static_cast<uint32_t>(-1);
			switch (D3DMemberTypeDesc.Type)
			{
				case D3D_SVT_BOOL:	MemberMeta.Type = USMConst_Bool; break;
				case D3D_SVT_INT:
				case D3D_SVT_UINT:	MemberMeta.Type = USMConst_Int; break;
				case D3D_SVT_FLOAT:	MemberMeta.Type = USMConst_Float; break;
				default:
				{
					if (pLog)
						pLog->LogInfo((std::string("Unsupported constant '") + pMemberName + "' type '" +
							std::to_string(D3DMemberTypeDesc.Type) + "' in a structure '" + (D3DTypeDesc.Name ? D3DTypeDesc.Name : "")).c_str());
					return false;
				}
			}

			MemberMeta.Columns = D3DMemberTypeDesc.Columns;
			MemberMeta.Rows = D3DMemberTypeDesc.Rows;

			if (D3DMemberTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
				MemberMeta.Flags |= USMShaderConst_ColumnMajor;
		}

		StructMeta.Members.push_back(std::move(MemberMeta));
	}

	Meta.Structs.push_back(std::move(StructMeta));

	return true;
}
//---------------------------------------------------------------------

bool ExtractUSMMetaFromBinary(const void* pData, size_t Size, CUSMShaderMeta& OutMeta, uint32_t& OutMinFeatureLevel, uint64_t& OutRequiresFlags, DEMShaderCompiler::ILogDelegate* pLog)
{
	ID3D11ShaderReflection* pReflector = nullptr;
	if (FAILED(D3DReflect(pData, Size, IID_ID3D11ShaderReflection, (void**)&pReflector))) return false;

	D3D_FEATURE_LEVEL D3DFeatureLevel;
	if (FAILED(pReflector->GetMinFeatureLevel(&D3DFeatureLevel)))
	{
		pReflector->Release();
		return false;
	}

	switch (D3DFeatureLevel)
	{
		case D3D_FEATURE_LEVEL_9_1:		OutMinFeatureLevel = GPU_Level_D3D9_1; break;
		case D3D_FEATURE_LEVEL_9_2:		OutMinFeatureLevel = GPU_Level_D3D9_2; break;
		case D3D_FEATURE_LEVEL_9_3:		OutMinFeatureLevel = GPU_Level_D3D9_3; break;
		case D3D_FEATURE_LEVEL_10_0:	OutMinFeatureLevel = GPU_Level_D3D10_0; break;
		case D3D_FEATURE_LEVEL_10_1:	OutMinFeatureLevel = GPU_Level_D3D10_1; break;
		case D3D_FEATURE_LEVEL_11_0:	OutMinFeatureLevel = GPU_Level_D3D11_0; break;
		//case D3D_FEATURE_LEVEL_11_1:
		default:						OutMinFeatureLevel = GPU_Level_D3D11_1; break;
	}

	OutRequiresFlags = pReflector->GetRequiresFlags();

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

				OutMeta.Resources.push_back(std::move(Meta));

				break;
			}
			case D3D_SIT_SAMPLER:
			{
				// D3D_SIF_COMPARISON_SAMPLER
				CUSMSamplerMeta Meta;
				Meta.Name = RsrcDesc.Name;
				Meta.RegisterStart = RsrcDesc.BindPoint;
				Meta.RegisterCount = RsrcDesc.BindCount;
				OutMeta.Samplers.push_back(std::move(Meta));
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
				OutMeta.Buffers.push_back(std::move(Meta));

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
						ConstMeta.BufferIndex = OutMeta.Buffers.size() - 1;
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
							if (!ProcessStructure(OutMeta, pVarType, ConstMeta.ElementSize, StructCache, pLog)) continue;
							ConstMeta.StructIndex = StructCache[pVarType];
						}
						else
						{
							ConstMeta.StructIndex = static_cast<uint32_t>(-1);
							switch (D3DTypeDesc.Type)
							{
								case D3D_SVT_BOOL:	ConstMeta.Type = USMConst_Bool; break;
								case D3D_SVT_INT:
								case D3D_SVT_UINT:	ConstMeta.Type = USMConst_Int; break;
								case D3D_SVT_FLOAT:	ConstMeta.Type = USMConst_Float; break;
								default:
								{
									if (pLog)
										pLog->LogInfo((std::string("Unsupported constant '") + D3DVarDesc.Name + "' type '" +
											std::to_string(D3DTypeDesc.Type) + "' in USM buffer '" + RsrcDesc.Name).c_str());
									return false;
								}
							}

							ConstMeta.Columns = D3DTypeDesc.Columns;
							ConstMeta.Rows = D3DTypeDesc.Rows;

							if (D3DTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
								ConstMeta.Flags |= USMShaderConst_ColumnMajor;

							OutMeta.Consts.push_back(std::move(ConstMeta));
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
