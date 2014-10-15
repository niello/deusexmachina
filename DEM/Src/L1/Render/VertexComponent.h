#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_COMPONENT_H__
#define __DEM_L1_RENDER_VERTEX_COMPONENT_H__

#include <System/System.h>

// Single component in a vertex layout

namespace Render
{

struct CVertexComponent
{
	enum ESemantic
	{
		Position = 0,
		Normal,
		Tangent,
		Bitangent,
		TexCoord,        
		Color,
		BoneWeights,
		BoneIndices,
		Invalid
	};

	enum EFormat
	{
		Float,		//> one-component float, expanded to (float, 0, 0, 1)
		Float2,		//> two-component float, expanded to (float, float, 0, 1)
		Float3,		//> three-component float, expanded to (float, float, float, 1)
		Float4,		//> four-component float
		UByte4,		//> four-component unsigned byte
		Short2,		//> two-component signed short, expanded to (value, value, 0, 1)
		Short4,		//> four-component signed short
		UByte4N,	//> four-component normalized unsigned byte (value / 255.0f)
		Short2N,	//> two-component normalized signed short (value / 32767.0f)
		Short4N,	//> four-component normalized signed short (value / 32767.0f)
		FORMAT_COUNT
	};

	static LPCSTR SemanticNames[];
	static LPCSTR FormatNames[];

	EFormat		Format;
	ESemantic	Semantic;
	DWORD		Index;
	DWORD		Stream;
	DWORD		OffsetInVertex;

	CVertexComponent(): Semantic(Invalid), Format(Float), Index(0), Stream(0) {}
	CVertexComponent(ESemantic Sem, EFormat Fmt, DWORD Idx = 0, DWORD StreamIdx = 0):
		Semantic(Sem), Format(Fmt), Index(Idx), Stream(StreamIdx) {}

	LPCSTR	GetFormatString() const { n_assert_dbg(Format < FORMAT_COUNT); return FormatNames[Format]; }
	LPCSTR	GetSemanticString() const { n_assert_dbg(Semantic < Invalid); return SemanticNames[Semantic]; }
	DWORD	GetSize() const;
};

inline DWORD CVertexComponent::GetSize() const
{
	switch (Format)
	{
		case Float:     return 4;
		case Float2:    return 8;
		case Float3:    return 12;
		case Float4:    return 16;
		case UByte4:    return 4;
		case Short2:    return 4;
		case Short4:    return 8;
		case UByte4N:   return 4;
		case Short2N:   return 4;
		case Short4N:   return 8;
	}
	Sys::Error("Invalid vertex component format!");
	return 0;
}
//---------------------------------------------------------------------

}

#endif
