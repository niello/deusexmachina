#include "ParamsSM30.h"
#include <ShaderMeta/SM30ShaderMeta.h>
#include <Utils.h>
#include <Logging.h>
#include <iostream>

//???!!!TODO:
//???skip loading shader metadata when creating effect in DEM? all relevant metadata is already copied to the effect.
//!!!struct member duplicates const meta, make membermeta = offset + constmeta?

struct CSM30EffectMeta
{
	// Param ID -> shader type mask + metadata
	std::vector<CSM30BufferMeta> Buffers;
	std::vector<CSM30StructMeta> Structs;
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

static bool CheckConstRegisterOverlapping(const CSM30ConstMeta& Param, const CSM30EffectMeta& Other)
{
	const auto& OtherRegs =
		(Param.RegisterSet == RS_Float4) ? Other.UsedFloat4 :
		(Param.RegisterSet == RS_Int4) ? Other.UsedInt4 :
		Other.UsedBool;

	// Fail if overlapping detected. Overlapping data can't be correctly set from effects.
	const auto RegisterEnd = Param.RegisterStart + Param.ElementRegisterCount * Param.ElementCount;
	for (uint32_t r = Param.RegisterStart; r < RegisterEnd; ++r)
		if (OtherRegs.find(r) != OtherRegs.cend())
			return false;

	return true;
}
//---------------------------------------------------------------------

static bool CheckSamplerRegisterOverlapping(const CSM30SamplerMeta& Param, const CSM30EffectMeta& Other)
{
	// Fail if overlapping detected. Overlapping data can't be correctly set from effects.
	for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
		if (Other.UsedSamplers.find(r) != Other.UsedSamplers.cend())
			return false;

	return true;
}
//---------------------------------------------------------------------

static bool ProcessConstant(uint8_t ShaderType, CSM30ConstMeta& Param, const CSM30ShaderMeta& SrcMeta, CSM30EffectMeta& TargetMeta, const CSM30EffectMeta& OtherMeta1, const CSM30EffectMeta& OtherMeta2, const CContext& Ctx)
{
	CStrID ID(Param.Name.c_str());

	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Consts.find(ID);
	if (ItPrev == TargetMeta.Consts.cend())
	{
		// Check register overlapping, because it prevents correct param setup from effects

		if (!CheckConstRegisterOverlapping(Param, OtherMeta1))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " constant '" << ID.CStr() << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (!CheckConstRegisterOverlapping(Param, OtherMeta2))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " constant '" << ID.CStr() << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		// Remember used registers for future overlap checks

		auto& Regs =
			(Param.RegisterSet == RS_Float4) ? TargetMeta.UsedFloat4 :
			(Param.RegisterSet == RS_Int4) ? TargetMeta.UsedInt4 :
			TargetMeta.UsedBool;

		const auto RegisterEnd = Param.RegisterStart + Param.ElementRegisterCount * Param.ElementCount;
		for (uint32_t r = Param.RegisterStart; r < RegisterEnd; ++r)
			Regs.insert(r);

		// Copy necessary metadata

		CSM30ConstMeta& NewConst = TargetMeta.Consts.emplace(ID, std::make_pair(ShaderType, std::move(Param))).first->second.second;
		CopyBufferMetadata(NewConst.BufferIndex, SrcMeta.Buffers, TargetMeta.Buffers);
		CopyStructMetadata(NewConst.StructIndex, SrcMeta.Structs, TargetMeta.Structs);
	}
	else
	{
		// The same param found, check compatibility
		const auto& ExistingMeta = ItPrev->second.second;
		if (ExistingMeta != Param)
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " constant '" << ID.CStr() << "' is not compatible across all tech shaders" << Ctx.LineEnd;
			return false;
		}

		// Compare containing constant buffers
		// NB: we perform this check despite D3D9 has no real constant buffers, because even with
		// the same set of registers we still can have different constant buffer layouts. Using
		// the constant with foreign constant buffer can be incompatible with the shader. D3D9 is
		// legacy and this situation will probably never happen, so I don't spend much time on solution.
		if (TargetMeta.Buffers[ExistingMeta.BufferIndex] != SrcMeta.Buffers[Param.BufferIndex])
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " param '" << ID.CStr() << "' containing constant buffer is not compatible across all tech shaders" << Ctx.LineEnd;
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderType;
	}

	return true;
}
//---------------------------------------------------------------------

static bool ProcessResource(uint8_t ShaderType, CSM30RsrcMeta& Param, CSM30EffectMeta& TargetMeta, const CSM30EffectMeta& OtherMeta1, const CSM30EffectMeta& OtherMeta2, const CContext& Ctx)
{
	CStrID ID(Param.Name.c_str());

	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Resources.find(ID);
	if (ItPrev == TargetMeta.Resources.cend())
	{
		if (OtherMeta1.UsedResources.find(Param.Register) != OtherMeta1.UsedResources.cend())
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " resource '" << ID.CStr() << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (OtherMeta2.UsedResources.find(Param.Register) != OtherMeta2.UsedResources.cend())
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " resource '" << ID.CStr() << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		TargetMeta.UsedResources.insert(Param.Register);

		TargetMeta.Resources.emplace(ID, std::make_pair(ShaderType, std::move(Param)));
	}
	else
	{
		// The same param found, check compatibility
		const auto& ExistingMeta = ItPrev->second.second;
		if (ExistingMeta != Param)
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " resource '" << ID.CStr() << "' is not compatible across all tech shaders" << Ctx.LineEnd;
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderType;
	}

	return true;
}
//---------------------------------------------------------------------

static bool ProcessSampler(uint8_t ShaderType, CSM30SamplerMeta& Param, CSM30EffectMeta& TargetMeta, const CSM30EffectMeta& OtherMeta1, const CSM30EffectMeta& OtherMeta2, const CContext& Ctx)
{
	CStrID ID(Param.Name.c_str());

	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Samplers.find(ID);
	if (ItPrev == TargetMeta.Samplers.cend())
	{
		if (!CheckSamplerRegisterOverlapping(Param, OtherMeta1))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " sampler '" << ID.CStr() << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (!CheckSamplerRegisterOverlapping(Param, OtherMeta2))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " sampler '" << ID.CStr() << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
			TargetMeta.UsedSamplers.insert(r);

		TargetMeta.Samplers.emplace(ID, std::make_pair(ShaderType, std::move(Param)));
	}
	else
	{
		// The same param found, check compatibility
		const auto& ExistingMeta = ItPrev->second.second;
		if (ExistingMeta != Param)
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " sampler '" << ID.CStr() << "' is not compatible across all tech shaders" << Ctx.LineEnd;
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderType;
	}

	return true;
}
//---------------------------------------------------------------------

bool WriteParameterTablesForDX9C(std::ostream& Stream, const std::vector<CTechnique>& Techs, const CContext& Ctx)
{
	CSM30EffectMeta GlobalMeta, MaterialMeta;
	GlobalMeta.PrintableName = "Global";
	MaterialMeta.PrintableName = "Material";

	for (const auto& Tech : Techs)
	{
		CSM30EffectMeta TechMeta;
		TechMeta.PrintableName = "Tech";

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
						ProcessConstant(ShaderType, Const, ShaderMeta, GlobalMeta, MaterialMeta, TechMeta, Ctx);
					}
					else
					{
						auto ItMtl = Ctx.MaterialParams.find(ID);
						if (ItMtl != Ctx.MaterialParams.cend())
						{
							ProcessConstant(ShaderType, Const, ShaderMeta, MaterialMeta, GlobalMeta, TechMeta, Ctx);

							// process defaults - null or depends on const type
							//???!!!support defaults for structs? CParams with fields!
						}
						else
						{
							ProcessConstant(ShaderType, Const, ShaderMeta, TechMeta, GlobalMeta, MaterialMeta, Ctx);
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
						ProcessResource(ShaderType, Rsrc, GlobalMeta, MaterialMeta, TechMeta, Ctx);
					}
					else
					{
						auto ItMtl = Ctx.MaterialParams.find(ID);
						if (ItMtl != Ctx.MaterialParams.cend())
						{
							ProcessResource(ShaderType, Rsrc, MaterialMeta, GlobalMeta, TechMeta, Ctx);

							// process defaults - null, string, CStrID
						}
						else
						{
							ProcessResource(ShaderType, Rsrc, TechMeta, GlobalMeta, MaterialMeta, Ctx);
						}
					}
				}

				for (auto& Sampler : ShaderMeta.Samplers)
				{
					if (Ctx.LogVerbosity >= EVerbosity::Debug)
						std::cout << "Shader '" << ShaderID.CStr() << "' sampler " << Sampler.Name << Ctx.LineEnd;

					CStrID ID(Sampler.Name.c_str());
					if (Ctx.GlobalParams.find(ID) != Ctx.GlobalParams.cend())
					{
						ProcessSampler(ShaderType, Sampler, GlobalMeta, MaterialMeta, TechMeta, Ctx);
					}
					else
					{
						auto ItMtl = Ctx.MaterialParams.find(ID);
						if (ItMtl != Ctx.MaterialParams.cend())
						{
							ProcessSampler(ShaderType, Sampler, MaterialMeta, GlobalMeta, TechMeta, Ctx);

							// process defaults - null or CParams
						}
						else
						{
							ProcessSampler(ShaderType, Sampler, TechMeta, GlobalMeta, MaterialMeta, Ctx);
						}
					}
				}
			}
		}
	}

	return false;
}
//---------------------------------------------------------------------
