#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_LAYOUT_H__
#define __DEM_L1_RENDER_VERTEX_LAYOUT_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>
#include <Render/D3D9Fwd.h>

// Vertex layout describes components of vertex buffer element

namespace Render
{

struct CVertexComponent
{
	enum ESemantic
	{
		Position = 0,
		Normal,
		Tangent,
		Binormal,
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

	EFormat		Format;
	ESemantic	Semantic;
	DWORD		Index;
	DWORD		Stream;
	DWORD		OffsetInVertex;

	CVertexComponent(): Semantic(Invalid), Format(Float), Index(0), Stream(0) {}
	CVertexComponent(ESemantic Sem, EFormat Fmt, DWORD Idx = 0, DWORD StreamIdx = 0):
		Semantic(Sem), Format(Fmt), Index(Idx), Stream(StreamIdx) {}

	LPCSTR	GetFormatString() const;
	BYTE	GetD3DDeclType() const;
	LPCSTR	GetSemanticString() const;
	BYTE	GetD3DUsage() const;
	DWORD	GetSize() const;
};

typedef Ptr<class CVertexLayout> PVertexLayout;

class CVertexLayout: public Core::CRefCounted
{
protected:

	CArray<CVertexComponent>		Components;
	DWORD							VertexSize;
	IDirect3DVertexDeclaration9*	pDecl;

public:

	CVertexLayout(): pDecl(NULL), VertexSize(0) {}
	~CVertexLayout() { Destroy(); }

	bool	Create(const CArray<CVertexComponent>& VertexComponents);
	void	Destroy();

	DWORD	GetVertexSize() const { return VertexSize; }

	static CStrID					BuildSignature(const CArray<CVertexComponent>& Components);
	const CArray<CVertexComponent>&	GetComponents() const { return Components; }
	IDirect3DVertexDeclaration9*	GetD3DVertexDeclaration() const { return pDecl; }
};

extern LPCSTR SemanticNames[];
extern BYTE SemanticD3DUsages[];
extern LPCSTR FormatNames[];
extern BYTE FormatD3DTypes[];

inline LPCSTR CVertexComponent::GetFormatString() const
{
	n_assert_dbg(Format < FORMAT_COUNT);
	return FormatNames[Format];
}
//---------------------------------------------------------------------

inline BYTE CVertexComponent::GetD3DDeclType() const
{
	n_assert_dbg(Format < FORMAT_COUNT);
	return FormatD3DTypes[Format];
}
//---------------------------------------------------------------------

inline LPCSTR CVertexComponent::GetSemanticString() const
{
	n_assert_dbg(Semantic < Invalid);
	return SemanticNames[Semantic];
}
//---------------------------------------------------------------------

inline BYTE CVertexComponent::GetD3DUsage() const
{
	n_assert_dbg(Semantic < Invalid);
	return SemanticD3DUsages[Semantic];
}
//---------------------------------------------------------------------

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
	Core::Error("Invalid vertex component format!");
	return 0;
}
//---------------------------------------------------------------------

}

#endif
