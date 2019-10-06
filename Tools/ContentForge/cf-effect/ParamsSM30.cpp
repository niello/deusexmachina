#include "ParamsSM30.h"
#include <ShaderMeta/SM30ShaderMeta.h>
#include <Utils.h>
#include <Logging.h>
#include <iostream>

bool WriteParameterTablesForDX9C(std::ostream& Stream, const std::vector<CTechnique>& Techs, const CContext& Ctx)
{
	const auto LineEnd = std::cout.widen('\n');

	struct CUsedSM30Registers
	{
		std::set<uint32_t> Float4;
		std::set<uint32_t> Int4;
		std::set<uint32_t> Bool;
		std::set<uint32_t> Resources;
		std::set<uint32_t> Samplers;
	};

	struct CSM30EffectMeta
	{
		std::map<CStrID, std::pair<uint8_t, CSM30BufferMeta>> Buffers;
		std::map<CStrID, std::pair<uint8_t, CSM30StructMeta>> Structs;
		std::map<CStrID, std::pair<uint8_t, CSM30ConstMeta>> Consts;
		std::map<CStrID, std::pair<uint8_t, CSM30RsrcMeta>> Resources;
		std::map<CStrID, std::pair<uint8_t, CSM30SamplerMeta>> Samplers;
	};

	//???the same for tech?
	CUsedSM30Registers GlobalRegisters, MaterialRegisters;

	//???need special meta with shader mask/type per each variable?
	//???map or vector of pairs [shader(s) -> meta]?
	CSM30EffectMeta GlobalMeta, MaterialMeta;

	for (const auto& Tech : Techs)
	{
		for (CStrID PassID : Tech.Passes)
		{
			const CRenderState& RS = Ctx.RSCache.at(PassID);

			CStrID ShaderIDs[] = { RS.VertexShader, RS.PixelShader, RS.GeometryShader, RS.HullShader, RS.DomainShader };

			for (uint8_t ShaderType = ShaderType_Vertex; ShaderType < ShaderType_COUNT; ++ShaderType)
			{
				const CStrID ShaderID = ShaderIDs[ShaderType];
				if (!ShaderID) continue;

				CSM30ShaderMeta ShaderMeta;
				{
					const CShaderData& ShaderData = Ctx.ShaderCache.at(ShaderID);
					membuf MetaBuffer(ShaderData.MetaBytes.get(), ShaderData.MetaBytes.get() + ShaderData.MetaByteCount);
					std::istream MetaStream(&MetaBuffer);
					MetaStream >> ShaderMeta;
				}

				for (auto& Const : ShaderMeta.Consts)
				{
					if (Ctx.LogVerbosity >= EVerbosity::Debug)
						std::cout << "Shader '" << ShaderID.CStr() << "' constant " << Const.Name << LineEnd;

					//???!!!related struct & buffer meta must be copied too?!

					CStrID ID(Const.Name.c_str());
					if (Ctx.GlobalParams.find(ID) != Ctx.GlobalParams.cend())
					{
						// Check if this const was already added from another shader
						auto ItPrev = GlobalMeta.Consts.find(ID);
						if (ItPrev == GlobalMeta.Consts.cend())
						{
							// Validate range against both material & tech registers, two calls of the common function
							/*
							CSM30ConstMeta* pSM30Const = (CSM30ConstMeta*)pMetaObject;
							CArray<UPTR>& UsedGlobalRegs = (pSM30Const->RegisterSet == RS_Float4) ? GlobalFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? GlobalInt4 : GlobalBool);
							CArray<UPTR>& UsedMaterialRegs = (pSM30Const->RegisterSet == RS_Float4) ? MaterialFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? MaterialInt4 : MaterialBool);
							for (UPTR r = pSM30Const->RegisterStart; r < pSM30Const->RegisterStart + pSM30Const->RegisterCount; ++r)
							{
							if (UsedMaterialRegs.Contains(r))
							{
							n_msg(VL_ERROR, "Global param '%s' uses a register used by material params\n", ParamID.CStr());
							return ERR_INVALID_DATA;
							}
							if (!UsedGlobalRegs.Contains(r)) UsedGlobalRegs.Add(r);
							}
							*/

							// New parameter, check registers and add
							GlobalMeta.Consts.emplace(ID, std::make_pair(ShaderType, std::move(Const)));
						}
						else
						{
							/*
							// The same param found, check compatibility

							const CEffectParam& Param = TechInfo.Params[Idx];
							const CMetadataObject* pExistingMetaObject = Param.GetMetadataObject();

							if (!pMetaObject->IsEqual(*pExistingMetaObject))
							{
							n_msg(VL_ERROR, "Tech '%s': param '%s' has different description in different shaders\n", TechInfo.ID.CStr(), MetaObjectID.CStr());
							return ERR_INVALID_DATA;
							}

							if (ShaderParamClass == ShaderParam_Const)
							{
							const CMetadataObject* pMetaBuffer = pMeta->GetContainingConstantBuffer(pMetaObject);
							const CMetadataObject* pExistingBuffer = Param.GetContainingBuffer();
							if (!pMetaBuffer->IsEqual(*pExistingBuffer)) //???must be equal or compatible?
							{
							n_msg(VL_ERROR, "Tech '%s': param '%s' containing buffers have different description in different shaders\n", TechInfo.ID.CStr(), MetaObjectID.CStr());
							return ERR_INVALID_DATA;
							}
							}
							*/

							// Existing parameter, check compatibility and extend shader mask
							ItPrev->second.first |= ShaderType;
						}
					}
					else
					{
						auto ItMtl = Ctx.MaterialParams.find(ID);
						if (ItMtl != Ctx.MaterialParams.cend())
						{
							//!!! check that registers don't overlap !
							//MaterialMeta.Consts.push_back(std::move(Const));

							// process defaults
						}
						else
						{
							////!!! check that registers don't overlap !
							//TechMeta.Consts.push_back(std::move(Const));
						}
					}
				}

				for (auto& Rsrc : ShaderMeta.Resources)
				{
					if (Ctx.LogVerbosity >= EVerbosity::Debug)
						std::cout << "Shader '" << ShaderID.CStr() << "' resource " << Rsrc.Name << LineEnd;

					CStrID ID(Rsrc.Name.c_str());
					if (Ctx.GlobalParams.find(ID) != Ctx.GlobalParams.cend())
					{
						//!!! check that registers don't overlap !
						//GlobalMeta.Resources.push_back(std::move(Rsrc));
					}
					else
					{
						auto ItMtl = Ctx.MaterialParams.find(ID);
						if (ItMtl != Ctx.MaterialParams.cend())
						{
							//!!! check that registers don't overlap !
							//MaterialMeta.Resources.push_back(std::move(Rsrc));

							// process defaults
						}
						else
						{
							////!!! check that registers don't overlap !
							//TechMeta.Resources.push_back(std::move(Rsrc));
						}
					}

					//in all cases check that registers don't overlap
				}

				for (auto& Sampler : ShaderMeta.Samplers)
				{
					if (Ctx.LogVerbosity >= EVerbosity::Debug)
						std::cout << "Shader '" << ShaderID.CStr() << "' sampler " << Sampler.Name << LineEnd;

					CStrID ID(Sampler.Name.c_str());
					if (Ctx.GlobalParams.find(ID) != Ctx.GlobalParams.cend())
					{
						//!!! check that registers don't overlap !
						//GlobalMeta.Samplers.push_back(std::move(Sampler));
					}
					else
					{
						auto ItMtl = Ctx.MaterialParams.find(ID);
						if (ItMtl != Ctx.MaterialParams.cend())
						{
							//!!! check that registers don't overlap !
							//MaterialMeta.Samplers.push_back(std::move(Sampler));

							// process defaults
						}
						else
						{
							//!!! check that registers don't overlap !
							//TechMeta.Samplers.push_back(std::move(Sampler));
						}
					}
				}
			}
		}
	}

	return false;
}
