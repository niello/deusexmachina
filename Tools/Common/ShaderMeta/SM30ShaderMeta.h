#pragma once
#include <vector>
#include <set>

// Legacy Shader Model 3.0 (SM3.0) metadata

// Don't change existing values, they are saved to file
enum ESM30ShaderConstFlags
{
	ShaderConst_ColumnMajor	= 0x01 // Only for matrix types
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

bool operator ==(const CSM30BufferMeta& a, const CSM30BufferMeta& b)
{
	// FIXME: why? was lazy to implement in a previous version?
	return true;
}

std::ostream& operator <<(std::ostream& Stream, const CSM30BufferMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30BufferMeta& Value);

struct CSM30StructMemberMeta
{
	std::string	Name;
	uint32_t	StructIndex;
	uint32_t	RegisterOffset;
	uint32_t	ElementRegisterCount;
	uint32_t	ElementCount;
	uint8_t		Columns;
	uint8_t		Rows;
	uint8_t		Flags;					// See ESM30ShaderConstFlags

	//???store register set and support mixed structs?
};

struct CSM30StructMeta
{
	//std::string						Name;
	std::vector<CSM30StructMemberMeta>	Members;
};

std::ostream& operator <<(std::ostream& Stream, const CSM30StructMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30StructMeta& Value);

// Arrays and single-type structures are supported
struct CSM30ConstMeta
{
	std::string			Name;
	uint32_t			BufferIndex;
	uint32_t			StructIndex;
	ESM30RegisterSet	RegisterSet;
	uint32_t			RegisterStart;
	uint32_t			ElementRegisterCount;
	uint32_t			ElementCount;
	uint8_t				Columns;
	uint8_t				Rows;
	uint8_t				Flags;					// See ESM30ShaderConstFlags

	uint32_t			RegisterCount;			// Cache, not saved
};

bool operator ==(const CSM30ConstMeta& a, const CSM30ConstMeta& b)
{
	return a.RegisterSet == b.RegisterSet &&
		a.RegisterStart == b.RegisterStart &&
		a.ElementRegisterCount == b.ElementRegisterCount &&
		a.ElementCount == b.ElementCount &&
		a.Flags == b.Flags;
}

std::ostream& operator <<(std::ostream& Stream, const CSM30ConstMeta& Value);
std::istream& operator >>(std::istream& Stream, CSM30ConstMeta& Value);

// Arrays aren't supported, one texture to multiple samplers isn't supported yet
//???what about one tex to multiple samplers? store register bit mask or 'UsedRegisters' array?
struct CSM30RsrcMeta
{
	std::string	Name;
	uint32_t	Register;
};

bool operator ==(const CSM30RsrcMeta& a, const CSM30RsrcMeta& b)
{
	return a.Register == b.Register;
}

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

bool operator ==(const CSM30SamplerMeta& a, const CSM30SamplerMeta& b)
{
	return a.Type == b.Type && a.RegisterStart == b.RegisterStart && a.RegisterCount == b.RegisterCount;
}

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
