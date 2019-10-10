#include <CFEffectFwd.h>
#include <ShaderMeta/USMShaderMeta.h>
#include <ParamsCommon.h>
#include <Utils.h>
#include <Logging.h>
#include <iostream>
#include <sstream>

//???!!!TODO:
//???skip loading shader metadata when creating effect in DEM? all relevant metadata is already copied to the effect.

struct CUSMEffectMeta
{
	// Order must be preserved, params reference them by index
	std::vector<CUSMBufferMeta> Buffers;
	std::vector<CUSMStructMeta> Structs;

	// Param ID (alphabetically sorted) -> shader type mask + metadata
	std::map<std::string, std::pair<uint8_t, CUSMConstMeta>> Consts;
	std::map<std::string, std::pair<uint8_t, CUSMRsrcMeta>> Resources;
	std::map<std::string, std::pair<uint8_t, CUSMSamplerMeta>> Samplers;

	// Cache for faster search
	std::set<uint32_t> UsedConstantBuffers;
	std::set<uint32_t> UsedResources;
	std::set<uint32_t> UsedSamplers;

	// For logging
	std::string PrintableName;
};

std::ostream& operator <<(std::ostream& Stream, const CUSMEffectMeta& Value)
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

static inline bool CheckRegisterOverlapping(uint32_t RegisterStart, uint32_t RegisterCount, const std::set<uint32_t>& Used)
{
	// Fail if overlapping detected. Overlapping data can't be correctly set from effects.
	for (uint32_t r = RegisterStart; r < RegisterStart + RegisterCount; ++r)
		if (Used.find(r) != Used.cend())
			return false;

	return true;
}
//---------------------------------------------------------------------

static void CopyBufferMetadata(uint32_t& BufferIndex, const std::vector<CUSMBufferMeta>& SrcBuffers, std::vector<CUSMBufferMeta>& TargetBuffers)
{
	if (BufferIndex == static_cast<uint32_t>(-1)) return;

	const auto& Buffer = SrcBuffers[BufferIndex];
	auto ItBuffer = std::find(TargetBuffers.cbegin(), TargetBuffers.cend(), Buffer);
	if (ItBuffer != TargetBuffers.cend())
	{
		// The same buffer found, reference it
		BufferIndex = static_cast<uint32_t>(std::distance(TargetBuffers.cbegin(), ItBuffer));
	}
	else
	{
		// Copy new buffer to metadata
		TargetBuffers.push_back(Buffer);
		BufferIndex = static_cast<uint32_t>(TargetBuffers.size() - 1);
	}
}
//---------------------------------------------------------------------

static bool ProcessConstant(uint8_t ShaderType, CUSMConstMeta& Param, const CUSMShaderMeta& SrcMeta, CUSMEffectMeta& TargetMeta, const CUSMEffectMeta& OtherMeta1, const CUSMEffectMeta& OtherMeta2, const CContext& Ctx)
{
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Consts.find(Param.Name);
	if (ItPrev == TargetMeta.Consts.cend())
	{
		// Check register overlapping, because it prevents correct param setup from effects

		const uint32_t BufferRegister = SrcMeta.Buffers[Param.BufferIndex].Register;

		if (OtherMeta1.UsedConstantBuffers.find(BufferRegister) != OtherMeta1.UsedConstantBuffers.cend())
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " buffer containing '" << Param.Name << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (OtherMeta2.UsedConstantBuffers.find(BufferRegister) != OtherMeta2.UsedConstantBuffers.cend())
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " buffer containing '" << Param.Name << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		TargetMeta.UsedConstantBuffers.insert(BufferRegister);

		// Copy necessary metadata

		std::string ParamName = Param.Name;
		CUSMConstMeta& NewConst = TargetMeta.Consts.emplace(std::move(ParamName), std::make_pair(ShaderType, std::move(Param))).first->second.second;
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
		if (TargetMeta.Buffers[ExistingMeta.BufferIndex] != SrcMeta.Buffers[Param.BufferIndex])
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " constant '" << Param.Name << "' containing buffer is not compatible across all tech shaders" << Ctx.LineEnd;
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderType;
	}

	return true;
}
//---------------------------------------------------------------------

static bool ProcessResource(uint8_t ShaderType, CUSMRsrcMeta& Param, CUSMEffectMeta& TargetMeta, const CUSMEffectMeta& OtherMeta1, const CUSMEffectMeta& OtherMeta2, const CContext& Ctx)
{
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Resources.find(Param.Name);
	if (ItPrev == TargetMeta.Resources.cend())
	{
		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, OtherMeta1.UsedResources))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " resource '" << Param.Name << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, OtherMeta2.UsedResources))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " resource '" << Param.Name << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
			TargetMeta.UsedResources.insert(r);

		std::string ParamName = Param.Name;
		TargetMeta.Resources.emplace(std::move(ParamName), std::make_pair(ShaderType, std::move(Param)));
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

static bool ProcessSampler(uint8_t ShaderType, CUSMSamplerMeta& Param, CUSMEffectMeta& TargetMeta, const CUSMEffectMeta& OtherMeta1, const CUSMEffectMeta& OtherMeta2, const CContext& Ctx)
{
	// Check if this param was already added from another shader
	auto ItPrev = TargetMeta.Samplers.find(Param.Name);
	if (ItPrev == TargetMeta.Samplers.cend())
	{
		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, OtherMeta1.UsedSamplers))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " sampler '" << Param.Name << "' uses a register used by " << OtherMeta1.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, OtherMeta2.UsedSamplers))
		{
			if (Ctx.LogVerbosity >= EVerbosity::Errors)
				std::cout << TargetMeta.PrintableName << " sampler '" << Param.Name << "' uses a register used by " << OtherMeta2.PrintableName << " params" << Ctx.LineEnd;
			return false;
		}

		for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
			TargetMeta.UsedSamplers.insert(r);

		std::string ParamName = Param.Name;
		TargetMeta.Samplers.emplace(std::move(ParamName), std::make_pair(ShaderType, std::move(Param)));
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
				if (Ctx.LogVerbosity >= EVerbosity::Warnings)
					std::cout << "Material param '" << Meta.Name << "' is a float, default value must be null, float, int or vector" << Ctx.LineEnd;
				return false;
			}
			break;
		}
		case USMConst_Int:
		{
			if (ValueSizeInBytes = WriteIntDefault(Stream, DefaultValue))
			{
				if (Ctx.LogVerbosity >= EVerbosity::Warnings)
					std::cout << "Material param '" << Meta.Name << "' is an integer, default value must be null, float, int or vector" << Ctx.LineEnd;
				return false;
			}
			break;
		}
		case USMConst_Bool:
		{
			if (ValueSizeInBytes = WriteBoolDefault(Stream, DefaultValue))
			{
				if (Ctx.LogVerbosity >= EVerbosity::Warnings)
					std::cout << "Material param '" << Meta.Name << "' is a bool, default value must be null, bool or int" << Ctx.LineEnd;
				return false;
			}
			break;
		}
		default:
		{
			if (Ctx.LogVerbosity >= EVerbosity::Warnings)
				std::cout << "Material param '" << Meta.Name << "' is a constant of unsupported type or register set, default value is skipped" << Ctx.LineEnd;
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
				if (Ctx.LogVerbosity >= EVerbosity::Warnings)
					std::cout << "Unsupported default type for resource '" << ID << "', must be string or string ID" << Ctx.LineEnd;
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
				if (Ctx.LogVerbosity >= EVerbosity::Warnings)
					std::cout << "Unsupported default type for sampler '" << ID << "', must be section with sampler settings" << Ctx.LineEnd;
			}
		}
		else
		{
			if (Ctx.LogVerbosity >= EVerbosity::Warnings)
				std::cout << "Default for unknow parameter '" << ID << "' is skipped" << Ctx.LineEnd;
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
