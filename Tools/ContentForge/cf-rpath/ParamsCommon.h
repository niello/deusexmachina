#include <Logging.h>
#include <vector>

template<class TEffectMeta>
bool CollectNonGlobalMetadataFromEffect(std::istream& Stream, std::vector<TEffectMeta>& MetaToCheck, CThreadSafeLog* pLog)
{
	TEffectMeta Buffer;

	// Add material params table for globals verification, skip defaults

	// Skip 32-bit metadata size
	ReadStream<uint32_t>(Stream);

	Stream >> Buffer;
	MetaToCheck.push_back(std::move(Buffer));

	// Get param tables from techniques

	const auto TechCount = ReadStream<uint32_t>(Stream);
	for (size_t i = 0; i < TechCount; ++i)
	{
		// Skip tech info to param table

		//ReadStream<std::string>(Stream); // ID not saved because it is not used in an engine
		ReadStream<uint32_t>(Stream); // MinFeatureLevel
		ReadStream<uint32_t>(Stream); // Offset
		ReadStream<std::string>(Stream); // InputSet

		const auto PassCount = ReadStream<uint32_t>(Stream);
		Stream.seekg(PassCount * sizeof(uint32_t), std::ios_base::cur);

		// Skip 32-bit metadata size
		ReadStream<uint32_t>(Stream);

		// Add tech params table for globals verification
		Stream >> Buffer;
		Buffer.PrintableName = "Tech"; // + std::to_string(i);
		MetaToCheck.push_back(std::move(Buffer));
	}

	return true;
}
//---------------------------------------------------------------------
