#pragma once
#include <vector>
#include <set>
#include <map>

// Legacy Shader Model 3.0 (SM3.0) metadata

// Don't change existing values, they are saved to file
enum ESM30ShaderConstFlags
{
	SM30ShaderConst_ColumnMajor	= 0x01 // Only for matrix types
};

// Don't change existing values, they are saved to file
enum ESM30SamplerType
{
	SM30Sampler_1D		= 0,
	SM30Sampler_2D,
	SM30Sampler_3D,
	SM30Sampler_CUBE
};

// Don't change existing values, they are saved to file
enum ESM30RegisterSet
{
	RS_Bool		= 0,
	RS_Int4		= 1,
	RS_Float4	= 2
};

struct CSM30BufferMeta
{
	std::string			Name;
	uint32_t			SlotIndex;	// Pseudo-register, two buffers at the same slot will conflict

	// NB: it is very important that they are sorted and have unique elements!
	std::set<uint32_t>	UsedFloat4;
	std::set<uint32_t>	UsedInt4;
	std::set<uint32_t>	UsedBool;
};

inline bool operator ==(const CSM30BufferMeta& a, const CSM30BufferMeta& b)
{
	return a.SlotIndex == b.SlotIndex && a.UsedFloat4 == b.UsedFloat4 && a.UsedInt4 == b.UsedInt4 && a.UsedBool == b.UsedBool;
}

inline bool operator !=(const CSM30BufferMeta& a, const CSM30BufferMeta& b) { return !(a == b); }
std::ostream& operator <<(std::ostream& Stream, const CSM30BufferMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30BufferMeta& Value);

struct CSM30ConstMetaBase
{
	std::string			Name;
	uint32_t			StructIndex;
	ESM30RegisterSet	RegisterSet; //???save for struct members and add mixed-type structure support?
	uint32_t			RegisterStart; // For structs - offset from the start of the structure
	uint32_t			ElementRegisterCount;
	uint32_t			ElementCount;
	uint8_t				Columns;
	uint8_t				Rows;
	uint8_t				Flags; // See ESM30ShaderConstFlags
};

// NB: StructIndex is not compared intentionally
inline bool operator ==(const CSM30ConstMetaBase& a, const CSM30ConstMetaBase& b)
{
	return a.RegisterSet == b.RegisterSet &&
		a.RegisterStart == b.RegisterStart &&
		a.ElementRegisterCount == b.ElementRegisterCount &&
		a.ElementCount == b.ElementCount &&
		a.Flags == b.Flags;
}

inline bool operator !=(const CSM30ConstMetaBase& a, const CSM30ConstMetaBase& b) { return !(a == b); }

struct CSM30StructMeta
{
	//std::string Name;
	std::vector<CSM30ConstMetaBase> Members;
};

std::ostream& operator <<(std::ostream& Stream, const CSM30StructMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30StructMeta& Value);

// Arrays and single-type structures are supported
struct CSM30ConstMeta : public CSM30ConstMetaBase
{
	uint32_t BufferIndex;
};

std::ostream& operator <<(std::ostream& Stream, const CSM30ConstMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30ConstMeta& Value);

// Arrays aren't supported, one texture to multiple samplers isn't supported yet
//???what about one tex to multiple samplers? store register bit mask or 'UsedRegisters' array?
struct CSM30RsrcMeta
{
	std::string	Name;
	uint32_t	Register;
};

inline bool operator ==(const CSM30RsrcMeta& a, const CSM30RsrcMeta& b)
{
	return a.Register == b.Register;
}

inline bool operator !=(const CSM30RsrcMeta& a, const CSM30RsrcMeta& b) { return !(a == b); }
std::ostream& operator <<(std::ostream& Stream, const CSM30RsrcMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30RsrcMeta& Value);

// Arrays supported with arbitrarily assigned textures
struct CSM30SamplerMeta
{
	std::string			Name;
	ESM30SamplerType	Type;
	uint32_t			RegisterStart;
	uint32_t			RegisterCount;
};

inline bool operator ==(const CSM30SamplerMeta& a, const CSM30SamplerMeta& b)
{
	return a.Type == b.Type && a.RegisterStart == b.RegisterStart && a.RegisterCount == b.RegisterCount;
}

inline bool operator !=(const CSM30SamplerMeta& a, const CSM30SamplerMeta& b) { return !(a == b); }
std::ostream& operator <<(std::ostream& Stream, const CSM30SamplerMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30SamplerMeta& Value);

struct CSM30ShaderMeta
{
	std::vector<CSM30BufferMeta>	Buffers;
	std::vector<CSM30StructMeta>	Structs;
	std::vector<CSM30ConstMeta>		Consts;
	std::vector<CSM30RsrcMeta>		Resources;
	std::vector<CSM30SamplerMeta>	Samplers;
};

std::ostream& operator <<(std::ostream& Stream, const CSM30ShaderMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30ShaderMeta& Value);

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

std::ostream& operator <<(std::ostream& Stream, const CSM30EffectMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30EffectMeta& Value);

uint32_t GetSerializedSize(const CSM30ShaderMeta& Value);
uint32_t GetSerializedSize(const CSM30EffectMeta& Value);
void MergeConstantBuffers(const CSM30BufferMeta& SrcBuffer, CSM30BufferMeta& TargetBuffer);
void CopyBufferMetadata(uint32_t& BufferIndex, const std::vector<CSM30BufferMeta>& SrcBuffers, std::vector<CSM30BufferMeta>& TargetBuffers);
bool CheckConstRegisterOverlapping(const CSM30ConstMeta& Param, const CSM30EffectMeta& Other);
