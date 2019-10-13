#include <CFRenderPathFwd.h>
#include <Render/ShaderMetaCommon.h>
#include <Render/USMShaderMeta.h>
#include <Logging.h>
#include <ParamsCommon.h>
#include <Utils.h>
#include <sstream>

bool MergeParams(CUSMEffectMeta& Src, CUSMEffectMeta& Dest, CThreadSafeLog* pLog)
{
	for (auto& Const : Src.Consts)
	{
		auto& Param = Const.second.second;

		auto ItPrev = Dest.Consts.find(Const.first);
		if (ItPrev == Dest.Consts.end())
		{
			Dest.UsedConstantBuffers.insert(Src.Buffers[Param.BufferIndex].Register);

			CopyBufferMetadata(Param.BufferIndex, Src.Buffers, Dest.Buffers);
			CopyStructMetadata(Param.StructIndex, Src.Structs, Dest.Structs);

			Dest.Consts.insert(std::move(Const));
		}
		else
		{
			// The same param found, check compatibility
			const auto& ExistingMeta = ItPrev->second.second;
			if (ExistingMeta != Param)
			{
				if (pLog) pLog->LogError("Constant '" + Param.Name + "' is not compatible across all effects");
				return false;
			}

			// Compare containing constant buffers
			if (Dest.Buffers[ExistingMeta.BufferIndex] != Src.Buffers[Param.BufferIndex])
			{
				if (pLog) pLog->LogError("Constant '" + Param.Name + "' containing buffer is not compatible across all effects");
				return false;
			}
		}
	}

	for (auto& Rsrc : Src.Resources)
	{
		auto& Param = Rsrc.second.second;

		auto ItPrev = Dest.Resources.find(Rsrc.first);
		if (ItPrev == Dest.Resources.end())
		{
			for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
				Dest.UsedResources.insert(r);

			Dest.Resources.insert(std::move(Rsrc));
		}
		else
		{
			// The same param found, check compatibility
			const auto& ExistingMeta = ItPrev->second.second;
			if (ExistingMeta != Param)
			{
				if (pLog) pLog->LogError("Resource '" + Param.Name + "' is not compatible across all effects");
				return false;
			}
		}
	}

	for (auto& Sampler : Src.Samplers)
	{
		auto& Param = Sampler.second.second;

		auto ItPrev = Dest.Samplers.find(Sampler.first);
		if (ItPrev == Dest.Samplers.end())
		{
			for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
				Dest.UsedSamplers.insert(r);

			Dest.Samplers.insert(std::move(Sampler));
		}
		else
		{
			// The same param found, check compatibility
			const auto& ExistingMeta = ItPrev->second.second;
			if (ExistingMeta != Param)
			{
				if (pLog) pLog->LogError("Sampler '" + Param.Name + "' is not compatible across all tech shaders");
				return false;
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool BuildGlobalsTableForDXBC(CGlobalTable& Task, CThreadSafeLog* pLog)
{
	CUSMEffectMeta RPGlobalMeta;
	RPGlobalMeta.PrintableName = "RP Global";

	// Load metadata from all effects, merge and verify compatibility

	std::vector<CUSMEffectMeta> MetaToCheck;

	for (const auto& Src : Task.Sources)
	{
		auto& InStream = *Src.Stream;

		InStream.seekg(Src.Offset, std::ios_base::beg);

		// Read effect's global params

		CUSMEffectMeta Buffer;
		Buffer.PrintableName = "Material";
		InStream >> Buffer;

		if (!MergeParams(Buffer, RPGlobalMeta, pLog)) return false;

		// Add material params table for globals verification
		InStream >> Buffer;
		MetaToCheck.push_back(std::move(Buffer));

		if (!SkipMaterialDefaults(InStream, MetaToCheck.back()))
		{
			if (pLog) pLog->LogError("Default for unknown material parameter is found");
			return false;
		}

		// Get param tables from techniques

		const auto TechCount = ReadStream<uint32_t>(InStream);
		for (size_t i = 0; i < TechCount; ++i)
		{
			// Skip tech info to param table

			//ReadStream<std::string>(InStream); // not used in an engine
			ReadStream<std::string>(InStream);
			ReadStream<uint32_t>(InStream);

			const auto PassCount = ReadStream<uint32_t>(InStream);
			InStream.seekg(PassCount * sizeof(uint32_t), std::ios_base::cur);

			// Add tech params table for globals verification
			InStream >> Buffer;
			Buffer.PrintableName = "Tech" + std::to_string(i);
			MetaToCheck.push_back(std::move(Buffer));
		}
	}

	// verify RPGlobalMeta against MetaToCheck

	std::ostringstream OutStream(std::ios_base::binary);
	OutStream << RPGlobalMeta;
	Task.Result = OutStream.str();

	return true;
}
//---------------------------------------------------------------------
