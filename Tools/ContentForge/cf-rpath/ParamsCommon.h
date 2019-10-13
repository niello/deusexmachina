#include <istream>

template<class TEffectMeta>
bool SkipMaterialDefaults(std::istream& Stream, const TEffectMeta& MaterialMeta)
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
			return false;
	}

	const auto DefaultsBufferSize = ReadStream<uint32_t>(Stream);
	Stream.seekg(DefaultsBufferSize, std::ios_base::cur);

	return true;
}
//---------------------------------------------------------------------
