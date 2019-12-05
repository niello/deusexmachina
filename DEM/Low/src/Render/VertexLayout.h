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

class CVertexLayout: public Core::CObject
{
	RTTI_CLASS_DECL;

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

	const CVertexComponent*			GetComponent(UPTR Index) const { return Index < Components.GetCount() ? &Components[Index] : nullptr; }
	UPTR							GetComponentCount() const { return Components.GetCount(); }
	UPTR							GetVertexSizeInBytes() const { return VertexSize; }
};

typedef Ptr<CVertexLayout> PVertexLayout;

}

#endif
