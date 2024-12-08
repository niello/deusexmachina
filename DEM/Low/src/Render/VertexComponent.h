#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_COMPONENT_H__
#define __DEM_L1_RENDER_VERTEX_COMPONENT_H__

#include <System/System.h>

// Single component in a vertex layout

namespace Render
{

// Don't change order and starting index
enum class EVertexComponentSemantic : U8
{
	Position = 0,
	Normal,
	Tangent,
	Bitangent,
	TexCoord,        
	Color,
	BoneWeights,
	BoneIndices,
	UserDefined,
	Invalid
};

// Don't change order and starting index
enum class EVertexComponentFormat : U8
{
	Float32_1,		//> one-component float, expanded to (float, 0, 0, 1)
	Float32_2,		//> two-component float, expanded to (float, float, 0, 1)
	Float32_3,		//> three-component float, expanded to (float, float, float, 1)
	Float32_4,		//> four-component float
	Float16_2,		//> Two 16-bit floating point values, expanded to (value, value, 0, 1)
	Float16_4,		//> Four 16-bit floating point values
	UInt8_4,		//> four-component unsigned byte
	UInt8_4_Norm,	//> four-component normalized unsigned byte (value / 255.0f)
	SInt16_2,		//> two-component signed short, expanded to (value, value, 0, 1)
	SInt16_4,		//> four-component signed short
	SInt16_2_Norm,	//> two-component normalized signed short (value / 32767.0f)
	SInt16_4_Norm,	//> four-component normalized signed short (value / 32767.0f)
	UInt16_2_Norm,	//> two-component normalized unsigned short (value / 65535.0f)
	UInt16_4_Norm,	//> four-component normalized unsigned short (value / 65535.0f)
	Invalid
};

constexpr U32 VertexComponentOffsetAuto(-1);

struct CVertexComponent
{
	static const char* SemanticNames[];
	static const char* FormatNames[];

	// FIXME: Order for compact packing? Or prefer clear declarations? Or use constructor?
	EVertexComponentSemantic	Semantic = EVertexComponentSemantic::Invalid;
	const char*					UserDefinedName = nullptr; // For UserDefined semantics
	U32							Index = 0;
	EVertexComponentFormat		Format = EVertexComponentFormat::Invalid;
	U32							Stream = 0;
	U32							OffsetInVertex = VertexComponentOffsetAuto;
	bool						PerInstanceData = false;

	const char*	GetSemanticString() const { n_assert_dbg(Semantic < EVertexComponentSemantic::Invalid); return Semantic >= EVertexComponentSemantic::UserDefined ? UserDefinedName : SemanticNames[static_cast<U8>(Semantic)]; }
	const char*	GetFormatString() const { n_assert_dbg(Format < EVertexComponentFormat::Invalid); return FormatNames[static_cast<U8>(Format)]; }
	UPTR		GetSize() const;
};

inline UPTR CVertexComponent::GetSize() const
{
	switch (Format)
	{
		case EVertexComponentFormat::Float32_1:		return 4;
		case EVertexComponentFormat::Float32_2:		return 8;
		case EVertexComponentFormat::Float32_3:		return 12;
		case EVertexComponentFormat::Float32_4:		return 16;
		case EVertexComponentFormat::Float16_2:		return 4;
		case EVertexComponentFormat::Float16_4:		return 8;
		case EVertexComponentFormat::UInt8_4:		return 4;
		case EVertexComponentFormat::UInt8_4_Norm:	return 4;
		case EVertexComponentFormat::SInt16_2:		return 4;
		case EVertexComponentFormat::SInt16_4:		return 8;
		case EVertexComponentFormat::SInt16_2_Norm:	return 4;
		case EVertexComponentFormat::SInt16_4_Norm:	return 8;
		case EVertexComponentFormat::UInt16_2_Norm:	return 4;
		case EVertexComponentFormat::UInt16_4_Norm:	return 8;
	}
	Sys::Error("Invalid vertex component format!");
	return 0;
}
//---------------------------------------------------------------------

}

#endif
