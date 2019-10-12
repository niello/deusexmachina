#include <CFEffectFwd.h>
#include <ShaderMeta/SM30ShaderMeta.h>
#include <ParamsCommon.h>
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

static void MergeConstantBuffers(const CSM30BufferMeta& SrcBuffer, CSM30BufferMeta& TargetBuffer)
{
	TargetBuffer.UsedFloat4.insert(SrcBuffer.UsedFloat4.begin(), SrcBuffer.UsedFloat4.end());
	TargetBuffer.UsedInt4.insert(SrcBuffer.UsedInt4.begin(), SrcBuffer.UsedInt4.end());
	TargetBuffer.UsedBool.insert(SrcBuffer.UsedBool.begin(), SrcBuffer.UsedBool.end());
}
//---------------------------------------------------------------------

static void CopyBufferMetadata(uint32_t& BufferIndex, const std::vector<CSM30BufferMeta>& SrcBuffers, std::vector<CSM30BufferMeta>& TargetBuffers)
{
	if (BufferIndex == static_cast<uint32_t>(-1)) return;

	const auto& Buffer = SrcBuffers[BufferIndex];
	auto ItBuffer = std::find(TargetBuffers.begin(), TargetBuffers.end(), Buffer);
	if (ItBuffer != TargetBuffers.end())
	{
		// The same buffer found, reference it
		BufferIndex = static_cast<uint32_t>(std::distance(TargetBuffers.begin(), ItBuffer));
		return;
	}

	// Buffer can differ due to registers of unused fields, try to merge if name is the same
	ItBuffer = std::find_if(TargetBuffers.begin(), TargetBuffers.end(), [&Buffer](const CSM30BufferMeta& TargetBuffer)
	{
		return Buffer.Name == TargetBuffer.Name && Buffer.SlotIndex == TargetBuffer.SlotIndex;
	});

	if (ItBuffer != TargetBuffers.end())
	{
		// Buffer with the same name and slot is found, merge them
		MergeConstantBuffers(Buffer, *ItBuffer);
		BufferIndex = static_cast<uint32_t>(std::distance(TargetBuffers.begin(), ItBuffer));
	}
	else
	{
		// Copy new buffer to metadata
		TargetBuffers.push_back(Buffer);
		BufferIndex = static_cast<uint32_t>(TargetBuffers.size() - 1);
	}
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
			Ctx.Log->LogError(TargetMeta.PrintableName + " constant '" + Param.Name + "' uses a register used by " + OtherMeta1.PrintableName + " params");
			return false;
		}

		if (!CheckConstRegisterOverlapping(Param, OtherMeta2))
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " constant '" + Param.Name + "' uses a register used by " + OtherMeta2.PrintableName + " params");
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

		std::string ParamName = Param.Name;
		CSM30ConstMeta& NewConst = TargetMeta.Consts.emplace(std::move(ParamName), std::make_pair(ShaderType, std::move(Param))).first->second.second;
		CopyBufferMetadata(NewConst.BufferIndex, SrcMeta.Buffers, TargetMeta.Buffers);
		CopyStructMetadata(NewConst.StructIndex, SrcMeta.Structs, TargetMeta.Structs);
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

		// Merge constant buffers instead of comparison, because unused parameters in some shaders
		// may render buffers inequal, but they are still compatible. Overlapping not checked because
		// it will be when adding constants. Probably it is insufficient but acceptable for legacy D3D9.
		MergeConstantBuffers(SrcMeta.Buffers[Param.BufferIndex], TargetMeta.Buffers[ExistingMeta.BufferIndex]);

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
			Ctx.Log->LogError(TargetMeta.PrintableName + " resource '" + Param.Name + "' uses a register used by " + OtherMeta1.PrintableName + " params");
			return false;
		}

		if (OtherMeta2.UsedResources.find(Param.Register) != OtherMeta2.UsedResources.cend())
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " resource '" + Param.Name + "' uses a register used by " + OtherMeta2.PrintableName + " params");
			return false;
		}

		TargetMeta.UsedResources.insert(Param.Register);

		std::string ParamName = Param.Name;
		TargetMeta.Resources.emplace(std::move(ParamName), std::make_pair(ShaderType, std::move(Param)));
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
			Ctx.Log->LogError(TargetMeta.PrintableName + " sampler '" + Param.Name + "' uses a register used by " + OtherMeta1.PrintableName + " params");
			return false;
		}

		if (!CheckSamplerRegisterOverlapping(Param, OtherMeta2))
		{
			Ctx.Log->LogError(TargetMeta.PrintableName + " sampler '" + Param.Name + "' uses a register used by " + OtherMeta2.PrintableName + " params");
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
			Ctx.Log->LogError(TargetMeta.PrintableName + " sampler '" + Param.Name + "' is not compatible across all tech shaders");
			return false;
		}

		// Extend shader mask
		ItPrev->second.first |= ShaderType;
	}

	return true;
}
//---------------------------------------------------------------------

static bool SerializeConstantDefault(std::ostream& Stream, const CSM30ConstMetaBase& Meta, const Data::CData& DefaultValue, const CContext& Ctx)
{
	if (DefaultValue.IsVoid()) return false;

	const uint32_t RegisterCount = Meta.ElementRegisterCount * Meta.ElementCount;
	if (!RegisterCount) return false;

	// TODO: support arrays and structures
	assert(Meta.ElementCount == 1 && Meta.StructIndex == static_cast<uint32_t>(-1));

	uint32_t ConstSizeInBytes = 0;
	uint32_t ValueSizeInBytes = 0;

	switch (Meta.RegisterSet)
	{
		case RS_Float4:
		{
			if (ValueSizeInBytes = WriteFloatDefault(Stream, DefaultValue))
			{
				Ctx.Log->LogWarning("Material param '" + Meta.Name + "' is a float, default value must be null, float, int or vector");
				return false;
			}

			ConstSizeInBytes = 4 * sizeof(float) * RegisterCount;
			break;
		}
		case RS_Int4:
		{
			if (ValueSizeInBytes = WriteIntDefault(Stream, DefaultValue))
			{
				Ctx.Log->LogWarning("Material param '" + Meta.Name + "' is an integer, default value must be null, float, int or vector");
				return false;
			}

			ConstSizeInBytes = 4 * sizeof(int) * RegisterCount;
			break;
		}
		case RS_Bool:
		{
			if (ValueSizeInBytes = WriteBoolDefault(Stream, DefaultValue))
			{
				Ctx.Log->LogWarning("Material param '" + Meta.Name + "' is a bool, default value must be null, bool or int");
				return false;
			}

			ConstSizeInBytes = sizeof(bool) * RegisterCount;
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
					Ctx.Log->LogDebug("Shader '" + ShaderID.ToString() + "' constant " + Const.Name);

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
					Ctx.Log->LogDebug("Shader '" + ShaderID.ToString() + "' resource " + Rsrc.Name);

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
					Ctx.Log->LogDebug("Shader '" + ShaderID.ToString() + "' sampler " + Sampler.Name);

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
			Ctx.Log->LogWarning("Default for unknown parameter '" + ID + "' is skipped");
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
