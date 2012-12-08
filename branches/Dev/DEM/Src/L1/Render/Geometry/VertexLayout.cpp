#include "VertexLayout.h"

#include <Render/Renderer.h>
#include <d3d9.h>

namespace Render
{

LPCSTR SemanticNames[] =
{
	"Pos",
	"Nrm",
	"Tgt",
	"Bnm",
	"Tex",        
	"Clr",
	"Bwh",
	"Bix"
};

//???D3DDECLUSAGE_PSIZE?
BYTE SemanticD3DUsages[] =
{
	D3DDECLUSAGE_POSITION,
	D3DDECLUSAGE_NORMAL,
	D3DDECLUSAGE_TANGENT,
	D3DDECLUSAGE_BINORMAL,
	D3DDECLUSAGE_TEXCOORD,        
	D3DDECLUSAGE_COLOR,
	D3DDECLUSAGE_BLENDWEIGHT,
	D3DDECLUSAGE_BLENDINDICES
};

BYTE FormatD3DTypes[] =
{
	D3DDECLTYPE_FLOAT1,
	D3DDECLTYPE_FLOAT2,
	D3DDECLTYPE_FLOAT3,
	D3DDECLTYPE_FLOAT4,
	D3DDECLTYPE_UBYTE4,        
	D3DDECLTYPE_SHORT2,
	D3DDECLTYPE_SHORT4,
	D3DDECLTYPE_UBYTE4N,
	D3DDECLTYPE_SHORT2N,
	D3DDECLTYPE_SHORT4N
};

bool CVertexLayout::Create(const nArray<CVertexComponent>& VertexComponents)
{
	n_assert(!pDecl && VertexComponents.Size());

	Components = VertexComponents;
	VertexSize = 0;
	for (int i = 0; i < Components.Size(); ++i)
	{
		Components[i].OffsetInVertex = VertexSize;
		VertexSize += Components[i].GetSize();
	}

	const DWORD MAX_VERTEX_COMPONENTS = 32;
	n_assert(Components.Size() < MAX_VERTEX_COMPONENTS);
	D3DVERTEXELEMENT9 DeclData[MAX_VERTEX_COMPONENTS] = { 0 };
	DWORD StreamOffset[CRenderer::MaxVertexStreamCount] = { 0 };
	int i = 0;
	for (i = 0; i < Components.Size(); i++)
	{
		const CVertexComponent& Component = Components[i];
		WORD StreamIndex = (WORD)Component.Stream;
		n_assert(StreamIndex < CRenderer::MaxVertexStreamCount);
		DeclData[i].Stream = StreamIndex;
		DeclData[i].Offset = (WORD)StreamOffset[StreamIndex];
		DeclData[i].Type = Component.GetD3DDeclType();
		DeclData[i].Method = D3DDECLMETHOD_DEFAULT;
		DeclData[i].Usage = Component.GetD3DUsage();
		DeclData[i].UsageIndex = (BYTE)Component.Index;
		StreamOffset[StreamIndex] += Component.GetSize();
	}
	DeclData[i].Stream = 0xff;
	DeclData[i].Type = (WORD)D3DDECLTYPE_UNUSED;

	return SUCCEEDED(RenderSrv->GetD3DDevice()->CreateVertexDeclaration(DeclData, &pDecl));
}
//---------------------------------------------------------------------

void CVertexLayout::Destroy()
{
	SAFE_RELEASE(pDecl);
	Components.Clear();
	VertexSize = 0;
}
//---------------------------------------------------------------------

}