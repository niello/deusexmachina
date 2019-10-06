#include "ParamsSM30.h"
#include <ShaderMeta/SM30ShaderMeta.h>
#include <Utils.h>
#include <Logging.h>
#include <iostream>

//???!!!TODO:
//???skip loading shader metadata when creating effect in DEM? all relevant metadata is already copied to the effect.

struct CSM30EffectMeta
{
	// Param ID -> shader type mask + metadata
	std::map<CStrID, std::pair<uint8_t, CSM30BufferMeta>> Buffers;
	std::map<CStrID, std::pair<uint8_t, CSM30StructMeta>> Structs;
	std::map<CStrID, std::pair<uint8_t, CSM30ConstMeta>> Consts;
	std::map<CStrID, std::pair<uint8_t, CSM30RsrcMeta>> Resources;
	std::map<CStrID, std::pair<uint8_t, CSM30SamplerMeta>> Samplers;

	// Cache for faster search
	std::set<uint32_t> UsedFloat4;
	std::set<uint32_t> UsedInt4;
	std::set<uint32_t> UsedBool;
	std::set<uint32_t> UsedResources;
	std::set<uint32_t> UsedSamplers;

	// For logging
	std::string PrintableName;
};

static bool CheckConstRegisterOverlapping(const CSM30ConstMeta& Param, CSM30EffectMeta& Meta, const CSM30EffectMeta& Other)
{
	auto& Regs =
		(Param.RegisterSet == RS_Float4) ? Meta.UsedFloat4 :
		(Param.RegisterSet == RS_Int4) ? Meta.UsedInt4 :
		Meta.UsedBool;

	const auto& OtherRegs =
		(Param.RegisterSet == RS_Float4) ? Other.UsedFloat4 :
		(Param.RegisterSet == RS_Int4) ? Other.UsedInt4 :
		Other.UsedBool;

	for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
	{
		// Fail if overlapping detected. Overlapping data can't be correctly set from effects.
		if (OtherRegs.find(r) != OtherRegs.cend()) return false;

		// Remember our own registers to check overlapping with other objects
		Regs.insert(r);
	}

	return true;
}

static bool ProcessNewConstant(uint8_t ShaderType, CSM30ConstMeta& Param, CSM30EffectMeta& TargetMeta, const CSM30EffectMeta& OtherMeta1, const CSM30EffectMeta& OtherMeta2, const CContext& Ctx)
{
	CStrID ID(Param.Name.c_str());

	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Consts.find(ID);
	if (ItPrev == TargetMeta.Consts.cend())
	{
		// New param, check register overlapping and add to meta

		if (!CheckConstRegisterOverlapping(Param, TargetMeta, OtherMeta1))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " param '" << ID.CStr() << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (!CheckConstRegisterOverlapping(Param, TargetMeta, OtherMeta2))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " param '" << ID.CStr() << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		TargetMeta.Consts.emplace(ID, std::make_pair(ShaderType, std::move(Param)));

		// Copy related struct and buffer metadata

		// TODO: copy necessary struct and buffer meta!
	}
	else
	{
		// The same param found, check compatibility

		if (ItPrev->second.second != Param)
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " param '" << ID.CStr() << "' is not compatible across all tech shaders" << Ctx.LineEnd;
			return false;
		}

		// Compare containing constant buffers (???must be equal or compatible?)

		/*
		const CMetadataObject* pMetaBuffer = pMeta->GetContainingConstantBuffer(pMetaObject);
		const CMetadataObject* pExistingBuffer = Param.GetContainingBuffer();
		if (!pMetaBuffer->IsEqual(*pExistingBuffer)) //???must be equal or compatible?
		{
			n_msg(VL_ERROR, "Tech '%s': param '%s' containing buffers have different description in different shaders\n", TechInfo.ID.CStr(), MetaObjectID.CStr());
			return ERR_INVALID_DATA;
		}
		*/

		// Extend shader mask
		ItPrev->second.first |= ShaderType;
	}

	return true;
}

bool WriteParameterTablesForDX9C(std::ostream& Stream, const std::vector<CTechnique>& Techs, const CContext& Ctx)
{
	CSM30EffectMeta GlobalMeta, MaterialMeta, TechMeta;
	GlobalMeta.PrintableName = "Global";
	MaterialMeta.PrintableName = "Material";
	TechMeta.PrintableName = "Tech";

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
						std::cout << "Shader '" << ShaderID.CStr() << "' constant " << Const.Name << Ctx.LineEnd;

					CStrID ID(Const.Name.c_str());
					if (Ctx.GlobalParams.find(ID) != Ctx.GlobalParams.cend())
					{
						ProcessNewConstant(ShaderType, Const, GlobalMeta, MaterialMeta, TechMeta, Ctx);
					}
					else
					{
						auto ItMtl = Ctx.MaterialParams.find(ID);
						if (ItMtl != Ctx.MaterialParams.cend())
						{
							ProcessNewConstant(ShaderType, Const, MaterialMeta, GlobalMeta, TechMeta, Ctx);

							// process defaults
						}
						else
						{
							ProcessNewConstant(ShaderType, Const, TechMeta, GlobalMeta, MaterialMeta, Ctx);
						}
					}
				}

				for (auto& Rsrc : ShaderMeta.Resources)
				{
					if (Ctx.LogVerbosity >= EVerbosity::Debug)
						std::cout << "Shader '" << ShaderID.CStr() << "' resource " << Rsrc.Name << Ctx.LineEnd;

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
						std::cout << "Shader '" << ShaderID.CStr() << "' sampler " << Sampler.Name << Ctx.LineEnd;

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
