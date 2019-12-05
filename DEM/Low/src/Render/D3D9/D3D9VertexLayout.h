#pragma once
#ifndef __DEM_L1_RENDER_D3D9_VERTEX_LAYOUT_H__
#define __DEM_L1_RENDER_D3D9_VERTEX_LAYOUT_H__

#include <Render/VertexLayout.h>

// Direct3D9 implementation of the vertex layout class

struct IDirect3DVertexDeclaration9;

namespace Render
{

class CD3D9VertexLayout: public CVertexLayout
{
	FACTORY_CLASS_DECL;

protected:

	IDirect3DVertexDeclaration9* pDecl = nullptr;
	U32                          InstanceStreamFlags = 0; // (1 << StreamIndex) is set if it is an instance data stream

	void InternalDestroy();

public:

	virtual ~CD3D9VertexLayout() { InternalDestroy(); }

	bool							Create(const CVertexComponent* pComponents, UPTR Count, IDirect3DVertexDeclaration9* pD3DDecl);
	virtual void					Destroy() { InternalDestroy(); CVertexLayout::InternalDestroy(); }
	virtual bool					IsValid() const { return !!pDecl; }

	IDirect3DVertexDeclaration9*	GetD3DVertexDeclaration() const { return pDecl; }
	U32								GetInstanceStreamFlags() const { return InstanceStreamFlags; }
};

typedef Ptr<CD3D9VertexLayout> PD3D9VertexLayout;

}

#endif
