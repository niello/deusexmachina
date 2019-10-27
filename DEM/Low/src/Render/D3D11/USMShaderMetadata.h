#pragma once
#include <Data/StringID.h>

// Universal Shader Model (4.0 and higher, for Direct3D 10 and higher) shader metadata

namespace Render
{

// Don't change values
enum EUSMBufferType
{
	USMBuffer_Constant		= 0,
	USMBuffer_Texture		= 1,
	USMBuffer_Structured	= 2
};

// Don't change values
enum EUSMConstType
{
	USMConst_Bool	= 0,
	USMConst_Int,
	USMConst_Float,

	USMConst_Struct,

	USMConst_Invalid
};

// Don't change values
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

// Don't change values
enum EUSMShaderConstFlags
{
	ShaderConst_ColumnMajor	= 0x01 // Only for matrix types
};

struct CUSMBufferMeta
{
	CStrID         Name;
	EUSMBufferType Type;
	uint32_t       Register;
	uint32_t       Size;		// For structured buffers - StructureByteStride
};

struct CUSMConstMetaBase
{
	CStrID        Name;

	//!!!
	uint32_t      StructIndex;

	EUSMConstType Type;
	uint32_t      Offset; // From the start of CB, for struct members - from the start of the structure
	uint32_t      ElementSize;
	uint32_t      ElementCount;
	uint8_t       Columns;
	uint8_t       Rows;
	uint8_t       Flags; // See EUSMShaderConstFlags
};

struct CUSMStructMeta
{
	CStrID Name;
	std::vector<CUSMConstMetaBase> Members;
};

struct CUSMConstMeta : public CUSMConstMetaBase
{
	uint32_t BufferIndex; //???ref?
};

struct CUSMRsrcMeta
{
	CStrID           Name;
	EUSMResourceType Type;
	uint32_t         RegisterStart;
	uint32_t         RegisterCount;
};

struct CUSMSamplerMeta
{
	CStrID   Name;
	uint32_t RegisterStart;
	uint32_t RegisterCount;
};

}
