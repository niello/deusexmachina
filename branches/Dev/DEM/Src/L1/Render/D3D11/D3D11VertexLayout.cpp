//#include "D3D9VertexLayout.h"
//
//#include <Render/RenderServer.h>
//
//namespace Render
//{
//
////???D3DDECLUSAGE_PSIZE?
//BYTE SemanticD3DUsages[] =
//{
//	D3DDECLUSAGE_POSITION,
//	D3DDECLUSAGE_NORMAL,
//	D3DDECLUSAGE_TANGENT,
//	D3DDECLUSAGE_BINORMAL,
//	D3DDECLUSAGE_TEXCOORD,        
//	D3DDECLUSAGE_COLOR,
//	D3DDECLUSAGE_BLENDWEIGHT,
//	D3DDECLUSAGE_BLENDINDICES
//};
//
//BYTE FormatD3DTypes[] =
//{
//	D3DDECLTYPE_FLOAT1,
//	D3DDECLTYPE_FLOAT2,
//	D3DDECLTYPE_FLOAT3,
//	D3DDECLTYPE_FLOAT4,
//	D3DDECLTYPE_UBYTE4,        
//	D3DDECLTYPE_SHORT2,
//	D3DDECLTYPE_SHORT4,
//	D3DDECLTYPE_UBYTE4N,
//	D3DDECLTYPE_SHORT2N,
//	D3DDECLTYPE_SHORT4N
//};
//
//bool CD3D9VertexLayout::Create(const CArray<CVertexComponent>& VertexComponents)
//{
//	n_assert(!pDecl && VertexComponents.GetCount());
//
//	Components = VertexComponents;
//	VertexSize = 0;
//	for (int i = 0; i < Components.GetCount(); ++i)
//	{
//		Components[i].OffsetInVertex = VertexSize;
//		VertexSize += Components[i].GetSize();
//	}
//
//	const DWORD MAX_VERTEX_COMPONENTS = 32;
//	n_assert(Components.GetCount() < MAX_VERTEX_COMPONENTS);
//	D3DVERTEXELEMENT9 DeclData[MAX_VERTEX_COMPONENTS] = { 0 };
//	DWORD StreamOffset[CRenderServer::MaxVertexStreamCount] = { 0 };
//	int i = 0;
//	for (i = 0; i < Components.GetCount(); i++)
//	{
//		const CVertexComponent& Component = Components[i];
//		WORD StreamIndex = (WORD)Component.Stream;
//		n_assert(StreamIndex < CRenderServer::MaxVertexStreamCount);
//		DeclData[i].Stream = StreamIndex;
//		DeclData[i].Offset = (WORD)StreamOffset[StreamIndex];
//		DeclData[i].Type = FormatD3DTypes[Component.Format];
//		DeclData[i].Method = D3DDECLMETHOD_DEFAULT;
//		DeclData[i].Usage = SemanticD3DUsages[Component.Semantic];
//		DeclData[i].UsageIndex = (BYTE)Component.Index;
//		StreamOffset[StreamIndex] += Component.GetSize();
//	}
//	DeclData[i].Stream = 0xff;
//	DeclData[i].Type = (WORD)D3DDECLTYPE_UNUSED;
//
//	return SUCCEEDED(RenderSrv->GetD3DDevice()->CreateVertexDeclaration(DeclData, &pDecl));
//}
////---------------------------------------------------------------------
//
//void CD3D9VertexLayout::InternalDestroy()
//{
//	SAFE_RELEASE(pDecl);
//}
////---------------------------------------------------------------------
//
//}