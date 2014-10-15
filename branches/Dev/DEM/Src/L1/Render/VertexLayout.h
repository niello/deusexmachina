#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_LAYOUT_H__
#define __DEM_L1_RENDER_VERTEX_LAYOUT_H__

#include <Core/Object.h>
#include <Render/VertexComponent.h>
#include <Data/StringID.h>

// Vertex layout describes components of a vertex in a vertex buffer(s)

namespace Render
{

class CVertexLayout: public Core::CObject
{
protected:

	CArray<CVertexComponent>	Components;
	DWORD						VertexSize;

	void InternalDestroy() { Components.Clear(); VertexSize = 0; }

public:

	CVertexLayout(): VertexSize(0) {}
	virtual ~CVertexLayout() { }

	virtual bool	Create(const CArray<CVertexComponent>& VertexComponents) = 0;
	virtual void	Destroy() { InternalDestroy(); }

	static CStrID					BuildSignature(const CArray<CVertexComponent>& Components);
	const CArray<CVertexComponent>&	GetComponents() const { return Components; }
	DWORD							GetVertexSize() const { return VertexSize; }
};

typedef Ptr<CVertexLayout> PVertexLayout;

}

#endif
