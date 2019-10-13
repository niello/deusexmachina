#include <CFRenderPathFwd.h>
#include <Render/ShaderMetaCommon.h>
#include <Render/USMShaderMeta.h>
#include <Logging.h>
#include <ParamsCommon.h>
#include <Utils.h>
#include <sstream>

bool MergeGlobals(CUSMEffectMeta& Src, CUSMEffectMeta& Dest, CThreadSafeLog* pLog)
{
	for (auto& Const : Src.Consts)
	{
		auto It = Dest.Consts.find(Const.first);
		if (It == Dest.Consts.end())
		{
			auto& Param = Const.second.second;

			Dest.UsedConstantBuffers.insert(Src.Buffers[Param.BufferIndex].Register);

			CopyBufferMetadata(Param.BufferIndex, Src.Buffers, Dest.Buffers);
			CopyStructMetadata(Param.StructIndex, Src.Structs, Dest.Structs);

			Dest.Consts.insert(std::move(Const));
		}
		else
		{
			// verify compatibility
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

		if (!MergeGlobals(Buffer, RPGlobalMeta, pLog)) return false;

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
