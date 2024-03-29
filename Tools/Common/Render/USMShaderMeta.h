#pragma once
#include "ShaderMetaCommon.h"

// Universal Shader Model (SM4.0 and higher) metadata

// Don't change existing values, they are saved to file
enum EUSMShaderConstFlags
{
	USMShaderConst_ColumnMajor	= 0x01 // Only for matrix types
};

// Don't change existing values, they are saved to file
enum EUSMBufferTypeMask
{
	USMBuffer_Texture		= (1 << 30),
	USMBuffer_Structured	= (2 << 30),

	USMBuffer_RegisterMask = ~(USMBuffer_Texture | USMBuffer_Structured)
};

// Don't change existing values, they are saved to file
enum EUSMConstType
{
	USMConst_Bool	= 0,
	USMConst_Int,
	USMConst_Float,

	USMConst_Struct,

	USMConst_Invalid
};

// Don't change existing values, they are saved to file
enum EUSMResourceType
{
	USMRsrc_Texture1D			= 0,
	USMRsrc_Texture1DArray,
	USMRsrc_Texture2D,
	USMRsrc_Texture2DArray,
	USMRsrc_Texture2DMS,
	USMRsrc_Texture2DMSArray,
	USMRsrc_Texture3D,
	USMRsrc_TextureCUBE,
	USMRsrc_TextureCUBEArray,

	USMRsrc_Unknown
};

struct CUSMBufferMeta
{
	std::string	Name;
	uint32_t	Register;
	uint32_t	Size;		// For structured buffers - StructureByteStride
};

inline bool operator ==(const CUSMBufferMeta& a, const CUSMBufferMeta& b)
{
	return a.Register == b.Register && a.Size == b.Size && a.Name == b.Name;
}

inline bool operator !=(const CUSMBufferMeta& a, const CUSMBufferMeta& b) { return !(a == b); }
std::ostream& operator <<(std::ostream& Stream, const CUSMBufferMeta& Value);
std::istream& operator >>(std::istream& Stream, CUSMBufferMeta& Value);

struct CUSMConstMetaBase
{
	std::string		Name;
	uint32_t		StructIndex;
	EUSMConstType	Type;
	uint32_t		Offset; // From the start of CB, for struct members - from the start of the structure
	uint32_t		ElementSize;
	uint32_t		ElementCount;
	uint8_t			Columns;
	uint8_t			Rows;
	uint8_t			Flags; // See EUSMShaderConstFlags
};

inline bool operator ==(const CUSMConstMetaBase& a, const CUSMConstMetaBase& b)
{
	return a.Type == b.Type && a.Offset == b.Offset && a.ElementSize == b.ElementSize && a.ElementCount == b.ElementCount;
}

inline bool operator !=(const CUSMConstMetaBase& a, const CUSMConstMetaBase& b) { return !(a == b); }

struct CUSMStructMeta
{
	std::string Name;
	std::vector<CUSMConstMetaBase> Members;
};

std::ostream& operator <<(std::ostream& Stream, const CUSMStructMeta& Value);
std::istream& operator >>(std::istream& Stream, CUSMStructMeta& Value);

// Arrays and mixed-type structs supported
struct CUSMConstMeta : public CUSMConstMetaBase
{
	uint32_t BufferIndex;
};

std::ostream& operator <<(std::ostream& Stream, const CUSMConstMeta& Value);
std::istream& operator >>(std::istream& Stream, CUSMConstMeta& Value);

struct CUSMRsrcMeta
{
	std::string Name;
	EUSMResourceType Type;
	uint32_t RegisterStart;
	uint32_t RegisterCount;
};

inline bool operator ==(const CUSMRsrcMeta& a, const CUSMRsrcMeta& b)
{
	return a.Type == b.Type && a.RegisterStart == b.RegisterStart && a.RegisterCount == b.RegisterCount;
}

inline bool operator !=(const CUSMRsrcMeta& a, const CUSMRsrcMeta& b) { return !(a == b); }
std::ostream& operator <<(std::ostream& Stream, const CUSMRsrcMeta& Value);
std::istream& operator >>(std::istream& Stream, CUSMRsrcMeta& Value);

struct CUSMSamplerMeta
{
	std::string Name;
	uint32_t RegisterStart;
	uint32_t RegisterCount;
};

inline bool operator ==(const CUSMSamplerMeta& a, const CUSMSamplerMeta& b)
{
	return a.RegisterStart == b.RegisterStart && a.RegisterCount == b.RegisterCount;
}

inline bool operator !=(const CUSMSamplerMeta& a, const CUSMSamplerMeta& b) { return !(a == b); }
std::ostream& operator <<(std::ostream& Stream, const CUSMSamplerMeta& Value);
std::istream& operator >>(std::istream& Stream, CUSMSamplerMeta& Value);

struct CUSMShaderMeta
{
	std::vector<CUSMBufferMeta> Buffers;
	std::vector<CUSMStructMeta> Structs;
	std::vector<CUSMConstMeta> Consts;
	std::vector<CUSMRsrcMeta> Resources;
	std::vector<CUSMSamplerMeta> Samplers;
};

std::ostream& operator <<(std::ostream& Stream, const CUSMShaderMeta& Value);
std::istream& operator >>(std::istream& Stream, CUSMShaderMeta& Value);

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

std::ostream& operator <<(std::ostream& Stream, const CUSMEffectMeta& Value);
std::istream& operator >>(std::istream& Stream, CUSMEffectMeta& Value);

uint32_t GetSerializedSize(const CUSMShaderMeta& Value);
uint32_t GetSerializedSize(const CUSMEffectMeta& Value);
void CopyBufferMetadata(uint32_t& BufferIndex, const std::vector<CUSMBufferMeta>& SrcBuffers, std::vector<CUSMBufferMeta>& TargetBuffers);
bool CollectMaterialParams(CMaterialParams& Out, const CUSMEffectMeta& Meta);
