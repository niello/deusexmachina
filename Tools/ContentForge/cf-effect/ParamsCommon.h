#include <Data.h>
#include <ostream>

uint32_t WriteFloatDefault(std::ostream& Stream, const Data::CData& DefaultValue);
uint32_t WriteIntDefault(std::ostream& Stream, const Data::CData& DefaultValue);
uint32_t WriteBoolDefault(std::ostream& Stream, const Data::CData& DefaultValue);
void SerializeSamplerState(std::ostream& Stream, const Data::CParams& SamplerDesc);