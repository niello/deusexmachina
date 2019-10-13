#include <Logging.h>
#include <vector>

template<class TEffectMeta>
bool SkipMaterialDefaults(std::istream& Stream, const TEffectMeta& MaterialMeta, CThreadSafeLog* pLog)
{
	const auto DefaultValueCount = ReadStream<uint32_t>(Stream);
	for (size_t i = 0; i < DefaultValueCount; ++i)
	{
		const auto ID = ReadStream<std::string>(Stream);
		if (MaterialMeta.Consts.find(ID) != MaterialMeta.Consts.cend())
			ReadStream<uint32_t>(Stream);
		else if (MaterialMeta.Resources.find(ID) != MaterialMeta.Resources.cend())
			ReadStream<std::string>(Stream);
		else if (MaterialMeta.Samplers.find(ID) != MaterialMeta.Samplers.cend())
			Stream.seekg(37, std::ios_base::cur); // Exactly 37 bytes in a binary sampler state
		else
		{
			if (pLog) pLog->LogError("Default for unknown material parameter '" + ID + "' is found");
			return false;
		}
	}

	const auto DefaultsBufferSize = ReadStream<uint32_t>(Stream);
	Stream.seekg(DefaultsBufferSize, std::ios_base::cur);

	return true;
}
//---------------------------------------------------------------------

template<class TEffectMeta>
bool CollectNonGlobalMetadataFromEffect(std::istream& Stream, std::vector<TEffectMeta>& MetaToCheck, CThreadSafeLog* pLog)
{
	TEffectMeta Buffer;

	// Add material params table for globals verification, skip defaults

	Stream >> Buffer;
	MetaToCheck.push_back(std::move(Buffer));

	if (!SkipMaterialDefaults(Stream, MetaToCheck.back(), pLog)) return false;

	// Get param tables from techniques

	const auto TechCount = ReadStream<uint32_t>(Stream);
	for (size_t i = 0; i < TechCount; ++i)
	{
		// Skip tech info to param table

		//ReadStream<std::string>(Stream); // not used in an engine
		ReadStream<std::string>(Stream);
		ReadStream<uint32_t>(Stream);

		const auto PassCount = ReadStream<uint32_t>(Stream);
		Stream.seekg(PassCount * sizeof(uint32_t), std::ios_base::cur);

		// Add tech params table for globals verification
		Stream >> Buffer;
		Buffer.PrintableName = "Tech"; // + std::to_string(i);
		MetaToCheck.push_back(std::move(Buffer));
	}

	return true;
}
//---------------------------------------------------------------------
