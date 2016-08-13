#pragma once
#ifndef __DEM_TOOLS_D3D9_SHADER_REFLECTION_H__
#define __DEM_TOOLS_D3D9_SHADER_REFLECTION_H__

#include <Data/Dictionary.h>
#include <Data/String.h>

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
	UPTR				Rows;
	UPTR				Columns;
	UPTR				Elements;				// For arrays
	UPTR				StructMembers;
	UPTR				Bytes;
	UPTR				ElementRegisterCount;	// For a single array element
};

struct CD3D9ConstantDesc
{
	CString						Name;
	CD3D9ConstantType			Type;
	EREGISTER_SET				RegisterSet;
	UPTR						RegisterIndex;
	UPTR						RegisterCount;
	//const void*				pDefaultValue; //???CBuffer or explicit mem mgmt? or don't read?
	CArray<CD3D9ConstantDesc>	Members;
};

bool D3D9Reflect(const void* pData, UPTR Size, CArray<CD3D9ConstantDesc>& OutConsts, CString& OutCreator);
void D3D9FindSamplerTextures(const char* pSrcText, CDict<CString, CArray<CString>>& OutSampToTex);
void D3D9FindConstantBuffer(const char* pSrcText, const CString& ConstName, CString& OutBufferName, U32& OutSlotIndex);

#endif
