#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_LAYOUT_H__
#define __DEM_L1_RENDER_VERTEX_LAYOUT_H__

#include <Core/Object.h>
#include <Render/VertexComponent.h>
#include <Data/FixedArray.h>
#include <Data/StringID.h>

// Vertex layout describes components of a vertex in a vertex buffer(s)
// It is enough to define one reusable vertex layout per format, so
// rendering system may cache layouts created once.

namespace Render
{

class CVertexLayout: public DEM::Core::CObject
{
	RTTI_CLASS_DECL(Render::CVertexLayout, DEM::Core::CObject);

protected:

	CFixedArray<CVertexComponent>	Components;
	UPTR							VertexSize;

	void InternalDestroy() { Components.Clear(); VertexSize = 0; }

public:

	CVertexLayout(): VertexSize(0) {}
	virtual ~CVertexLayout() { InternalDestroy(); }

	static CStrID					BuildSignature(const CVertexComponent* pComponents, UPTR Count);

	virtual void					Destroy() { InternalDestroy(); }
	virtual bool					IsValid() const = 0;

	const CVertexComponent*			GetComponent(UPTR Index) const { return Index < Components.size() ? &Components[Index] : nullptr; }
	UPTR							GetComponentCount() const { return Components.size(); }
	UPTR							GetVertexSizeInBytes() const { return VertexSize; }
};

typedef Ptr<CVertexLayout> PVertexLayout;

}

#endif
