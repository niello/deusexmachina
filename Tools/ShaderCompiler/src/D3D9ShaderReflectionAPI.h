#pragma once
#include <string>
#include <vector>
#include <map>

// Code is obtained from
// http://www.gamedev.net/topic/648016-replacement-for-id3dxconstanttable/
// This version has some cosmetic changes and enhancements based on Wine d3dx9_36.dll code

enum EREGISTER_SET
{
	RS_BOOL,
	RS_INT4,
	RS_FLOAT4,
	RS_SAMPLER,

	// Added
	RS_MIXED	// For mixed-type structures
};

enum EPARAMETER_CLASS
{
	PC_SCALAR,
	PC_VECTOR,
	PC_MATRIX_ROWS,
	PC_MATRIX_COLUMNS,
	PC_OBJECT,
	PC_STRUCT
};
 
enum EPARAMETER_TYPE
{
	PT_VOID,
	PT_BOOL,
	PT_INT,
	PT_FLOAT,
	PT_STRING,
	PT_TEXTURE,
	PT_TEXTURE1D,
	PT_TEXTURE2D,
	PT_TEXTURE3D,
	PT_TEXTURECUBE,
	PT_SAMPLER,
	PT_SAMPLER1D,
	PT_SAMPLER2D,
	PT_SAMPLER3D,
	PT_SAMPLERCUBE,
	PT_PIXELSHADER,
	PT_VERTEXSHADER,
	PT_PIXELFRAGMENT,
	PT_VERTEXFRAGMENT,
	PT_UNSUPPORTED,

	// Added
	PT_MIXED	// For mixed-type structures
};

struct CD3D9ConstantType
{
	EPARAMETER_CLASS	Class;
	EPARAMETER_TYPE		Type;
	uint32_t			Rows;
	uint32_t			Columns;
	uint32_t			Elements;				// For arrays
	uint32_t			StructMembers;
	uint32_t			Bytes;
	uint32_t			ElementRegisterCount;	// For a single array element
};

struct CD3D9ConstantDesc
{
	std::string					Name;
	CD3D9ConstantType			Type;
	uint32_t					StructID;
	EREGISTER_SET				RegisterSet;
	uint32_t					RegisterIndex;
	uint32_t					RegisterCount;
	//const void*				pDefaultValue; //???CBuffer or explicit mem mgmt? or don't read?
};

struct CD3D9StructDesc
{
	size_t						Bytes;
	EPARAMETER_TYPE				Type;
	std::vector<CD3D9ConstantDesc>	Members;
};

bool D3D9Reflect(const void* pData, size_t Size, std::vector<CD3D9ConstantDesc>& OutConsts, std::map<uint32_t, CD3D9StructDesc>& OutStructs, std::string& OutCreator);
void D3D9FindSamplerTextures(const char* pSrcText, std::map<std::string, std::vector<std::string>>& OutSampToTex);
void D3D9FindConstantBuffer(const char* pSrcText, const std::string& ConstName, std::string& OutBufferName, uint32_t& OutSlotIndex);
