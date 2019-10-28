#include <CFEffectFwd.h>
#include <Render/USMShaderMeta.h>
#include <Utils.h>
#include <Logging.h>
#include <iostream>
#include <sstream>

static bool ProcessConstant(uint8_t ShaderTypeMask, CUSMConstMeta& Param, const CUSMShaderMeta& SrcMeta, CUSMEffectMeta& TargetMeta, const CUSMEffectMeta& OtherMeta1, const CUSMEffectMeta& OtherMeta2, const CContext& Ctx)
{
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Consts.find(Param.Name);
	if (ItPrev == TargetMeta.Consts.cend())
	{
		// Check register overlapping, because it prevents correct param setup from effects

		const uint32_t BufferRegister = SrcMeta.Buffers[Param.BufferIndex].Register;

		if (OtherMeta1.UsedConstantBuffers.find(BufferRegister) != OtherMeta1.UsedConstantBuffers.cend())
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " buffer containing '" + Param.Name + "' uses a register used by " + OtherMeta1.PrintableName + " params");
			return false;
		}

		if (OtherMeta2.UsedConstantBuffers.find(BufferRegister) != OtherMeta2.UsedConstantBuffers.cend())
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " buffer containing '" + Param.Name + "' uses a register used by " + OtherMeta2.PrintableName + " params");
			return false;
		}

		TargetMeta.UsedConstantBuffers.insert(BufferRegister);

		// Copy necessary metadata

		CopyBufferMetadata(Param.BufferIndex, SrcMeta.Buffers, TargetMeta.Buffers);
		CopyStructMetadata(Param.StructIndex, SrcMeta.Structs, TargetMeta.Structs);

		std::string ParamName = Param.Name;
		TargetMeta.Consts.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}
	else
	{
		// The same param found, check compatibility
		const auto& ExistingMeta = ItPrev->second.second;
		if (ExistingMeta != Param)
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " constant '" + Param.Name + "' is not compatible across all tech shaders");
			return false;
		}

		// Compare containing constant buffers
		if (TargetMeta.Buffers[ExistingMeta.BufferIndex] != SrcMeta.Buffers[Param.BufferIndex])
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " constant '" + Param.Name + "' containing buffer is not compatible across all tech shaders");
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderTypeMask;
	}

	return true;
}
//---------------------------------------------------------------------

static bool ProcessResource(uint8_t ShaderTypeMask, CUSMRsrcMeta& Param, CUSMEffectMeta& TargetMeta, const CUSMEffectMeta& OtherMeta1, const CUSMEffectMeta& OtherMeta2, const CContext& Ctx)
{
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Resources.find(Param.Name);
	if (ItPrev == TargetMeta.Resources.cend())
	{
		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, OtherMeta1.UsedResources))
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " resource '" + Param.Name + "' uses a register used by " + OtherMeta1.PrintableName + " params");
			return false;
		}

		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, OtherMeta2.UsedResources))
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " resource '" + Param.Name + "' uses a register used by " + OtherMeta2.PrintableName + " params");
			return false;
		}

		for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
			TargetMeta.UsedResources.insert(r);

		std::string ParamName = Param.Name;
		TargetMeta.Resources.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}
	else
	{
		// The same param found, check compatibility
		const auto& ExistingMeta = ItPrev->second.second;
		if (ExistingMeta != Param)
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " resource '" + Param.Name + "' is not compatible across all tech shaders");
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderTypeMask;
	}

	return true;
}
//---------------------------------------------------------------------

static bool ProcessSampler(uint8_t ShaderTypeMask, CUSMSamplerMeta& Param, CUSMEffectMeta& TargetMeta, const CUSMEffectMeta& OtherMeta1, const CUSMEffectMeta& OtherMeta2, const CContext& Ctx)
{
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Samplers.find(Param.Name);
	if (ItPrev == TargetMeta.Samplers.cend())
	{
		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, OtherMeta1.UsedSamplers))
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " sampler '" + Param.Name + "' uses a register used by " + OtherMeta1.PrintableName + " params");
			return false;
		}

		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, OtherMeta2.UsedSamplers))
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " sampler '" + Param.Name + "' uses a register used by " + OtherMeta2.PrintableName + " params");
			return false;
		}

		for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
			TargetMeta.UsedSamplers.insert(r);

		std::string ParamName = Param.Name;
		TargetMeta.Samplers.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}
	else
	{
		// The same param found, check compatibility
		const auto& ExistingMeta = ItPrev->second.second;
		if (ExistingMeta != Param)
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " sampler '" + Param.Name + "' is not compatible across all tech shaders");
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderTypeMask;
	}

	return true;
}
//---------------------------------------------------------------------

bool WriteParameterTablesForDXBC(std::ostream& Stream, std::vector<CTechnique>& Techs, CMaterialParams& MaterialParams, const CContext& Ctx)
{
	CUSMEffectMeta GlobalMeta, MaterialMeta;
	GlobalMeta.PrintableName = "Global";
	MaterialMeta.PrintableName = "Material";

	// Scan all shaders and collect global, material and per-technique parameters metadata

	for (auto& Tech : Techs)
	{
		CUSMEffectMeta TechMeta;
		TechMeta.PrintableName = "Tech";

		for (CStrID PassID : Tech.Passes)
		{
			const CRenderState& RS = Ctx.RSCache.at(PassID);

			CStrID ShaderIDs[] = { RS.VertexShader, RS.PixelShader, RS.GeometryShader, RS.HullShader, RS.DomainShader };

			for (uint8_t ShaderType = ShaderType_Vertex; ShaderType < ShaderType_COUNT; ++ShaderType)
			{
				const CStrID ShaderID = ShaderIDs[ShaderType];
				if (!ShaderID) continue;

				CUSMShaderMeta ShaderMeta;
				{
					const CShaderData& ShaderData = Ctx.ShaderCache.at(ShaderID);
					membuf MetaBuffer(ShaderData.MetaBytes.get(), ShaderData.MetaBytes.get() + ShaderData.MetaByteCount);
					std::istream MetaStream(&MetaBuffer);

					// Skip additional metadata (input signature ID, requires flags)
					ReadStream<uint32_t>(MetaStream);
					ReadStream<uint64_t>(MetaStream);

					MetaStream >> ShaderMeta;
				}

				const uint8_t ShaderTypeMask = (1 << ShaderType);

				for (auto& Const : ShaderMeta.Consts)
				{
					Ctx.Log->LogDebug("Shader '" + ShaderID.ToString() + "' constant " + Const.Name);

					CStrID ID(Const.Name.c_str());
					if (Ctx.GlobalParams.find(ID) != Ctx.GlobalParams.cend())
					{
						if (!ProcessConstant(ShaderTypeMask, Const, ShaderMeta, GlobalMeta, MaterialMeta, TechMeta, Ctx)) return false;
					}
					else if (Ctx.MaterialParams.find(ID) != Ctx.MaterialParams.cend())
					{
						if (!ProcessConstant(ShaderTypeMask, Const, ShaderMeta, MaterialMeta, GlobalMeta, TechMeta, Ctx)) return false;
					}
					else
					{
						if (!ProcessConstant(ShaderTypeMask, Const, ShaderMeta, TechMeta, GlobalMeta, MaterialMeta, Ctx)) return false;
					}
				}

				for (auto& Rsrc : ShaderMeta.Resources)
				{
					Ctx.Log->LogDebug("Shader '" + ShaderID.ToString() + "' resource " + Rsrc.Name);

					CStrID ID(Rsrc.Name.c_str());
					if (Ctx.GlobalParams.find(ID) != Ctx.GlobalParams.cend())
					{
						if (!ProcessResource(ShaderTypeMask, Rsrc, GlobalMeta, MaterialMeta, TechMeta, Ctx)) return false;
					}
					else if (Ctx.MaterialParams.find(ID) != Ctx.MaterialParams.cend())
					{
						if (!ProcessResource(ShaderTypeMask, Rsrc, MaterialMeta, GlobalMeta, TechMeta, Ctx)) return false;
					}
					else
					{
						if (!ProcessResource(ShaderTypeMask, Rsrc, TechMeta, GlobalMeta, MaterialMeta, Ctx)) return false;
					}
				}

				for (auto& Sampler : ShaderMeta.Samplers)
				{
					Ctx.Log->LogDebug("Shader '" + ShaderID.ToString() + "' sampler " + Sampler.Name);

					CStrID ID(Sampler.Name.c_str());
					if (Ctx.GlobalParams.find(ID) != Ctx.GlobalParams.cend())
					{
						if (!ProcessSampler(ShaderTypeMask, Sampler, GlobalMeta, MaterialMeta, TechMeta, Ctx)) return false;
					}
					else if (Ctx.MaterialParams.find(ID) != Ctx.MaterialParams.cend())
					{
						if (!ProcessSampler(ShaderTypeMask, Sampler, MaterialMeta, GlobalMeta, TechMeta, Ctx)) return false;
					}
					else
					{
						if (!ProcessSampler(ShaderTypeMask, Sampler, TechMeta, GlobalMeta, MaterialMeta, Ctx)) return false;
					}
				}
			}
		}

		// Will be serialized to file with other tech data in calling code
		{
			const uint32_t MetaSize = GetSerializedSize(TechMeta);

			std::ostringstream TechStream(std::ios_base::binary);
			WriteStream<uint32_t>(TechStream, MetaSize);
			TechStream << TechMeta;
			Tech.EffectMetaBinary = TechStream.str();

			assert(Tech.EffectMetaBinary.size() == MetaSize + sizeof(uint32_t));
		}
	}

	// Serialize global and material tables to output

	{
		const uint32_t MetaSize = GetSerializedSize(GlobalMeta);
		const auto MetaStart = Stream.tellp();

		WriteStream<uint32_t>(Stream, MetaSize);
		Stream << GlobalMeta;

		assert(Stream.tellp() == MetaStart + std::streampos(MetaSize + sizeof(uint32_t)));
	}

	{
		const uint32_t MetaSize = GetSerializedSize(MaterialMeta);
		const auto MetaStart = Stream.tellp();

		WriteStream<uint32_t>(Stream, MetaSize);
		Stream << MaterialMeta;

		assert(Stream.tellp() == MetaStart + std::streampos(MetaSize + sizeof(uint32_t)));
	}

	// Build table for material defaults

	if (!CollectMaterialParams(MaterialParams, MaterialMeta))
	{
		Ctx.Log->LogError("Material metadata is incompatible across different shader formats");
		return false;
	}

	return true;
}
//---------------------------------------------------------------------
