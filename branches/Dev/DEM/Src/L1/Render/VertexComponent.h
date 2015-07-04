#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_COMPONENT_H__
#define __DEM_L1_RENDER_VERTEX_COMPONENT_H__

#include <System/System.h>

// Single component in a vertex layout

namespace Render
{

enum EVertexComponentSemantic
{
	VCSem_Position = 0,
	VCSem_Normal,
	VCSem_Tangent,
	VCSem_Bitangent,
	VCSem_TexCoord,        
	VCSem_Color,
	VCSem_BoneWeights,
	VCSem_BoneIndices,
	VCSem_UserDefined,
	VCSem_Invalid
};

enum EVertexComponentFormat
{
	VCFmt_Float32_1,		//> one-component float, expanded to (float, 0, 0, 1)
	VCFmt_Float32_2,		//> two-component float, expanded to (float, float, 0, 1)
	VCFmt_Float32_3,		//> three-component float, expanded to (float, float, float, 1)
	VCFmt_Float32_4,		//> four-component float
	VCFmt_Float16_2,		//> Two 16-bit floating point values, expanded to (value, value, 0, 1)
	VCFmt_Float16_4,		//> Four 16-bit floating point values
	VCFmt_UInt8_4,			//> four-component unsigned byte
	VCFmt_UInt8_4_Norm,		//> four-component normalized unsigned byte (value / 255.0f)
	VCFmt_SInt16_2,			//> two-component signed short, expanded to (value, value, 0, 1)
	VCFmt_SInt16_4,			//> four-component signed short
	VCFmt_SInt16_2_Norm,	//> two-component normalized signed short (value / 32767.0f)
	VCFmt_SInt16_4_Norm,	//> four-component normalized signed short (value / 32767.0f)
	VCFmt_UInt16_2_Norm,	//> two-component normalized unsigned short (value / 65535.0f)
	VCFmt_UInt16_4_Norm,	//> four-component normalized unsigned short (value / 65535.0f)
	VCFmt_Invalid
};

#define DEM_VERTEX_COMPONENT_OFFSET_DEFAULT ((DWORD)-1)

struct CVertexComponent
{
	static LPCSTR SemanticNames[];
	static LPCSTR FormatNames[];

	EVertexComponentSemantic	Semantic;
	LPCSTR						UserDefinedName; // For UserDefined semantics
	DWORD						Index;
	EVertexComponentFormat		Format;
	DWORD						Stream;
	DWORD						OffsetInVertex;

	LPCSTR	GetSemanticString() const { n_assert_dbg(Semantic < VCSem_Invalid); return Semantic >= VCSem_UserDefined ? UserDefinedName : SemanticNames[Semantic]; }
	LPCSTR	GetFormatString() const { n_assert_dbg(Format < VCFmt_Invalid); return FormatNames[Format]; }
	DWORD	GetSize() const;
};

inline DWORD CVertexComponent::GetSize() const
{
	switch (Format)
	{
		case VCFmt_Float32_1:		return 4;
		case VCFmt_Float32_2:		return 8;
		case VCFmt_Float32_3:		return 12;
		case VCFmt_Float32_4:		return 16;
		case VCFmt_Float16_2:		return 4;
		case VCFmt_Float16_4:		return 8;
		case VCFmt_UInt8_4:			return 4;
		case VCFmt_UInt8_4_Norm:	return 4;
		case VCFmt_SInt16_2:		return 4;
		case VCFmt_SInt16_4:		return 8;
		case VCFmt_SInt16_2_Norm:	return 4;
		case VCFmt_SInt16_4_Norm:	return 8;
		case VCFmt_UInt16_2_Norm:	return 4;
		case VCFmt_UInt16_4_Norm:	return 8;
	}
	Sys::Error("Invalid vertex component format!");
	return 0;
}
//---------------------------------------------------------------------

}

#endif
