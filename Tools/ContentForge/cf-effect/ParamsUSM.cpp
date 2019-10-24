#include <CFEffectFwd.h>
#include <Render/ShaderMetaCommon.h>
#include <Render/USMShaderMeta.h>
#include <ParamsCommon.h>
#include <Utils.h>
#include <Logging.h>
#include <iostream>
#include <sstream>

//???!!!TODO:
//???skip loading shader metadata when creating effect in DEM? all relevant metadata is already copied to the effect.

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

static bool SerializeConstantDefault(std::ostream& Stream, const CUSMConstMetaBase& Meta, const Data::CData& DefaultValue, const CContext& Ctx)
{
	if (DefaultValue.IsVoid()) return false;

	const uint32_t ConstSizeInBytes = Meta.ElementSize * Meta.ElementCount;
	if (!ConstSizeInBytes) return false;

	// TODO: support structures
	assert(Meta.Type != USMConst_Struct);

	uint32_t ValueSizeInBytes = 0;

	switch (Meta.Type)
	{
		case USMConst_Float:
		{
			if (ValueSizeInBytes = WriteFloatDefault(Stream, DefaultValue))
			{
				Ctx.Log->LogWarning("Material param '" + Meta.Name + "' is a float, default value must be null, float, int or vector");
				return false;
			}
			break;
		}
		case USMConst_Int:
		{
			if (ValueSizeInBytes = WriteIntDefault(Stream, DefaultValue))
			{
				Ctx.Log->LogWarning("Material param '" + Meta.Name + "' is an integer, default value must be null, float, int or vector");
				return false;
			}
			break;
		}
		case USMConst_Bool:
		{
			if (ValueSizeInBytes = WriteBoolDefault(Stream, DefaultValue))
			{
				Ctx.Log->LogWarning("Material param '" + Meta.Name + "' is a bool, default value must be null, bool or int");
				return false;
			}
			break;
		}
		default:
		{
			Ctx.Log->LogWarning("Material param '" + Meta.Name + "' is a constant of unsupported type or register set, default value is skipped");
			return false;
		}
	}

	for (uint32_t i = ValueSizeInBytes; i < ConstSizeInBytes; ++i)
		WriteStream<uint8_t>(Stream, 0);

	return true;
}
//---------------------------------------------------------------------

bool WriteParameterTablesForDXBC(std::ostream& Stream, std::vector<CTechnique>& Techs, const CContext& Ctx)
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
			TechStream << TechMeta;
			Tech.EffectMetaBinary = TechStream.str();
			assert(Tech.EffectMetaBinary.size() == MetaSize);
		}
	}

	// Serialize global and material tables to output

	{
		const uint32_t MetaSize = GetSerializedSize(GlobalMeta);
		const auto MetaStart = Stream.tellp();
		Stream << GlobalMeta;
		assert(Stream.tellp() == MetaStart + std::streampos(MetaSize));
	}

	{
		const uint32_t MetaSize = GetSerializedSize(MaterialMeta);
		const auto MetaStart = Stream.tellp();
		Stream << MaterialMeta;
		assert(Stream.tellp() == MetaStart + std::streampos(MetaSize));
	}

	// Serialize material defaults

	const auto DefaultCountOffset = Stream.tellp();
	WriteStream<uint32_t>(Stream, 0);

	uint32_t DefaultCount = 0;
	std::ostringstream DefaultConstsStream(std::ios_base::binary);

	for (const auto& MaterialParam : Ctx.MaterialParams)
	{
		const std::string ID = MaterialParam.first.CStr();
		auto ItConst = MaterialMeta.Consts.find(ID);
		if (ItConst != MaterialMeta.Consts.cend())
		{
			const uint32_t CurrDefaultOffset = static_cast<uint32_t>(DefaultConstsStream.tellp());

			if (!SerializeConstantDefault(DefaultConstsStream, ItConst->second.second, MaterialParam.second, Ctx)) continue;

			WriteStream(Stream, ID);
			WriteStream(Stream, CurrDefaultOffset);
			++DefaultCount;
		}
		else if (MaterialMeta.Resources.find(ID) != MaterialMeta.Resources.cend())
		{
			if (MaterialParam.second.IsA<CStrID>())
			{
				WriteStream(Stream, ID);
				WriteStream(Stream, MaterialParam.second.GetValue<CStrID>().ToString());
				++DefaultCount;
			}
			else if (MaterialParam.second.IsA<std::string>())
			{
				WriteStream(Stream, ID);
				WriteStream(Stream, MaterialParam.second.GetValue<std::string>());
				++DefaultCount;
			}
			else if (!MaterialParam.second.IsVoid())
			{
				Ctx.Log->LogWarning("Unsupported default type for resource '" + ID + "', must be string or string ID");
			}
		}
		else if (MaterialMeta.Samplers.find(ID) != MaterialMeta.Samplers.cend())
		{
			if (MaterialParam.second.IsA<Data::CParams>())
			{
				WriteStream(Stream, ID);
				SerializeSamplerState(Stream, MaterialParam.second.GetValue<Data::CParams>());
				++DefaultCount;
			}
			else if (!MaterialParam.second.IsVoid())
			{
				Ctx.Log->LogWarning("Unsupported default type for sampler '" + ID + "', must be section with sampler settings");
			}
		}
		else
		{
			Ctx.Log->LogWarning("Default for unknown material parameter '" + ID + "' is skipped");
		}
	}

	// Write actual default value count

	const auto EndOffset = Stream.tellp();
	Stream.seekp(DefaultCountOffset, std::ios_base::beg);
	WriteStream<uint32_t>(Stream, DefaultCount);
	Stream.seekp(EndOffset, std::ios_base::beg);

	// Write the buffer with constant defaults

	const std::string DefaultConstsBuffer = DefaultConstsStream.str();
	WriteStream(Stream, static_cast<uint32_t>(DefaultConstsBuffer.size()));
	if (!DefaultConstsBuffer.empty())
		Stream.write(DefaultConstsBuffer.c_str(), DefaultConstsBuffer.size());

	return true;
}
//---------------------------------------------------------------------
