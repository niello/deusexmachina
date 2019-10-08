#include "ParamsSM30.h"
#include <ShaderMeta/SM30ShaderMeta.h>
#include <RenderEnums.h> // For sampler desc. To common code?
#include <Utils.h>
#include <Logging.h>
#include <iostream>
#include <sstream>

//???!!!TODO:
//???skip loading shader metadata when creating effect in DEM? all relevant metadata is already copied to the effect.

struct CSM30EffectMeta
{
	// Order must be preserved, params reference them by index
	std::vector<CSM30BufferMeta> Buffers;
	std::vector<CSM30StructMeta> Structs;

	// Param ID (alphabetically sorted) -> shader type mask + metadata
	std::map<std::string, std::pair<uint8_t, CSM30ConstMeta>> Consts;
	std::map<std::string, std::pair<uint8_t, CSM30RsrcMeta>> Resources;
	std::map<std::string, std::pair<uint8_t, CSM30SamplerMeta>> Samplers;

	// Cache for faster search
	std::set<uint32_t> UsedFloat4;
	std::set<uint32_t> UsedInt4;
	std::set<uint32_t> UsedBool;
	std::set<uint32_t> UsedResources;
	std::set<uint32_t> UsedSamplers;

	// For logging
	std::string PrintableName;
};

std::ostream& operator <<(std::ostream& Stream, const CSM30EffectMeta& Value)
{
	WriteStream<uint32_t>(Stream, Value.Buffers.size());
	for (const auto& Obj : Value.Buffers)
		Stream << Obj;

	WriteStream<uint32_t>(Stream, Value.Structs.size());
	for (const auto& Obj : Value.Structs)
		Stream << Obj;

	WriteStream<uint32_t>(Stream, Value.Consts.size());
	for (const auto& IDToMeta : Value.Consts)
	{
		WriteStream(Stream, IDToMeta.first);
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	WriteStream<uint32_t>(Stream, Value.Resources.size());
	for (const auto& IDToMeta : Value.Resources)
	{
		WriteStream(Stream, IDToMeta.first);
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	WriteStream<uint32_t>(Stream, Value.Samplers.size());
	for (const auto& IDToMeta : Value.Samplers)
	{
		WriteStream(Stream, IDToMeta.first);
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	return Stream;
}
//---------------------------------------------------------------------

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
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Consts.find(Param.Name);
	if (ItPrev == TargetMeta.Consts.cend())
	{
		// Check register overlapping, because it prevents correct param setup from effects

		if (!CheckConstRegisterOverlapping(Param, OtherMeta1))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " constant '" << Param.Name << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (!CheckConstRegisterOverlapping(Param, OtherMeta2))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " constant '" << Param.Name << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
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

		CSM30ConstMeta& NewConst = TargetMeta.Consts.emplace(Param.Name, std::make_pair(ShaderType, std::move(Param))).first->second.second;
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
				std::cout << TargetMeta.PrintableName << " constant '" << Param.Name << "' is not compatible across all tech shaders" << Ctx.LineEnd;
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
				std::cout << TargetMeta.PrintableName << " param '" << Param.Name << "' containing constant buffer is not compatible across all tech shaders" << Ctx.LineEnd;
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
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Resources.find(Param.Name);
	if (ItPrev == TargetMeta.Resources.cend())
	{
		if (OtherMeta1.UsedResources.find(Param.Register) != OtherMeta1.UsedResources.cend())
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " resource '" << Param.Name << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (OtherMeta2.UsedResources.find(Param.Register) != OtherMeta2.UsedResources.cend())
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " resource '" << Param.Name << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		TargetMeta.UsedResources.insert(Param.Register);

		TargetMeta.Resources.emplace(Param.Name, std::make_pair(ShaderType, std::move(Param)));
	}
	else
	{
		// The same param found, check compatibility
		const auto& ExistingMeta = ItPrev->second.second;
		if (ExistingMeta != Param)
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " resource '" << Param.Name << "' is not compatible across all tech shaders" << Ctx.LineEnd;
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
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Samplers.find(Param.Name);
	if (ItPrev == TargetMeta.Samplers.cend())
	{
		if (!CheckSamplerRegisterOverlapping(Param, OtherMeta1))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " sampler '" << Param.Name << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (!CheckSamplerRegisterOverlapping(Param, OtherMeta2))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " sampler '" << Param.Name << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
			TargetMeta.UsedSamplers.insert(r);

		TargetMeta.Samplers.emplace(Param.Name, std::make_pair(ShaderType, std::move(Param)));
	}
	else
	{
		// The same param found, check compatibility
		const auto& ExistingMeta = ItPrev->second.second;
		if (ExistingMeta != Param)
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " sampler '" << Param.Name << "' is not compatible across all tech shaders" << Ctx.LineEnd;
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderType;
	}

	return true;
}
//---------------------------------------------------------------------

// TODO: common code!
static void SerializeSamplerState(std::ostream& Stream, const Data::CParams& SamplerDesc)
{
	// NB: default values are different for D3D9 and D3D11, but we use the same defaults for consistency

	std::string StrValue;
	vector4 ColorRGBA;

	if (TryGetParam(StrValue, SamplerDesc, "AddressU"))
		WriteStream<uint8_t>(Stream, StringToTexAddressMode(StrValue));
	else
		WriteStream<uint8_t>(Stream, TexAddr_Wrap);

	if (TryGetParam(StrValue, SamplerDesc, "AddressV"))
		WriteStream<uint8_t>(Stream, StringToTexAddressMode(StrValue));
	else
		WriteStream<uint8_t>(Stream, TexAddr_Wrap);

	if (TryGetParam(StrValue, SamplerDesc, "AddressW"))
		WriteStream<uint8_t>(Stream, StringToTexAddressMode(StrValue));
	else
		WriteStream<uint8_t>(Stream, TexAddr_Wrap);

	if (TryGetParam(StrValue, SamplerDesc, "Filter"))
		WriteStream<uint8_t>(Stream, StringToTexFilter(StrValue));
	else
		WriteStream<uint8_t>(Stream, TexFilter_MinMagMip_Linear);

	if (!TryGetParam(ColorRGBA, SamplerDesc, "BorderColorRGBA"))
		ColorRGBA = { 0.f, 0.f, 0.f, 0.f };

	WriteStream(Stream, ColorRGBA);

	WriteStream(Stream, GetParam(SamplerDesc, "MipMapLODBias", 0.f));
	WriteStream(Stream, GetParam(SamplerDesc, "FinestMipMapLOD", 0.f));
	WriteStream(Stream, GetParam(SamplerDesc, "CoarsestMipMapLOD", std::numeric_limits<float>().max()));
	WriteStream<uint32_t>(Stream, GetParam<int>(SamplerDesc, "MaxAnisotropy", 1));

	if (TryGetParam(StrValue, SamplerDesc, "CmpFunc"))
		WriteStream<uint8_t>(Stream, StringToCmpFunc(StrValue));
	else
		WriteStream<uint8_t>(Stream, Cmp_Never);
}
//---------------------------------------------------------------------

bool WriteParameterTablesForDX9C(std::ostream& Stream, std::vector<CTechnique>& Techs, const CContext& Ctx)
{
	CSM30EffectMeta GlobalMeta, MaterialMeta;
	GlobalMeta.PrintableName = "Global";
	MaterialMeta.PrintableName = "Material";

	// Scan all shaders and collect global, material and per-technique parameters metadata

	for (auto& Tech : Techs)
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
					else if (Ctx.MaterialParams.find(ID) != Ctx.MaterialParams.cend())
					{
						ProcessConstant(ShaderType, Const, ShaderMeta, MaterialMeta, GlobalMeta, TechMeta, Ctx);
					}
					else
					{
						ProcessConstant(ShaderType, Const, ShaderMeta, TechMeta, GlobalMeta, MaterialMeta, Ctx);
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
					else if (Ctx.MaterialParams.find(ID) != Ctx.MaterialParams.cend())
					{
						ProcessResource(ShaderType, Rsrc, MaterialMeta, GlobalMeta, TechMeta, Ctx);
					}
					else
					{
						ProcessResource(ShaderType, Rsrc, TechMeta, GlobalMeta, MaterialMeta, Ctx);
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
					else if (Ctx.MaterialParams.find(ID) != Ctx.MaterialParams.cend())
					{
						ProcessSampler(ShaderType, Sampler, MaterialMeta, GlobalMeta, TechMeta, Ctx);
					}
					else
					{
						ProcessSampler(ShaderType, Sampler, TechMeta, GlobalMeta, MaterialMeta, Ctx);
					}
				}
			}
		}

		// Will be serialized to file with other tech data in calling code
		{
			std::ostringstream TechStream(std::ios_base::binary);
			TechStream << TechMeta;
			Tech.EffectMetaBinary = TechStream.str();
		}
	}

	// Serialize global and material tables to output

	Stream << GlobalMeta;
	Stream << MaterialMeta;

	// Serialize material defaults

	const auto DefaultCountOffset = Stream.tellp();
	WriteStream<uint32_t>(Stream, 0);

	uint32_t DefaultCount = 0;

	for (const auto& MaterialParam : Ctx.MaterialParams)
	{
		const std::string ID = MaterialParam.first.CStr();
		if (MaterialMeta.Consts.find(ID) != MaterialMeta.Consts.cend())
		{
			// process defaults - null or depends on const type
			//???!!!support defaults for structs? CParams with fields!
		}
		else if (MaterialMeta.Resources.find(ID) != MaterialMeta.Resources.cend())
		{
			if (MaterialParam.second.IsA<CStrID>())
			{
				WriteStream(Stream, ID);
				WriteStream(Stream, MaterialParam.second.GetValue<CStrID>().ToString());
			}
			else if (MaterialParam.second.IsA<std::string>())
			{
				WriteStream(Stream, ID);
				WriteStream(Stream, MaterialParam.second.GetValue<std::string>());
			}
			else if (!MaterialParam.second.IsVoid())
			{
				if (Ctx.LogVerbosity >= EVerbosity::Warnings)
					std::cout << "Unsupported default type for resource '" << ID << "', must be string or string ID" << Ctx.LineEnd;
				continue;
			}
		}
		else if (MaterialMeta.Samplers.find(ID) != MaterialMeta.Samplers.cend())
		{
			if (MaterialParam.second.IsA<Data::CParams>())
			{
				WriteStream(Stream, ID);
				SerializeSamplerState(Stream, MaterialParam.second.GetValue<Data::CParams>());
			}
			else if (!MaterialParam.second.IsVoid())
			{
				if (Ctx.LogVerbosity >= EVerbosity::Warnings)
					std::cout << "Unsupported default type for sampler '" << ID << "', must be section with sampler settings" << Ctx.LineEnd;
				continue;
			}
		}
		else
		{
			if (Ctx.LogVerbosity >= EVerbosity::Warnings)
				std::cout << "Default for unknow parameter '" << ID << "' is skipped" << Ctx.LineEnd;
			continue;
		}

		++DefaultCount;
	}

	const auto EndOffset = Stream.tellp();
	Stream.seekp(DefaultCountOffset, std::ios_base::beg);
	WriteStream<uint32_t>(Stream, DefaultCount);
	Stream.seekp(EndOffset, std::ios_base::beg);

	return true;
}
//---------------------------------------------------------------------
