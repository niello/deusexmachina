#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_H__
#define __DEM_L1_RENDER_RENDERER_H__

#include <Core/RTTIBaseClass.h>
#include <Data/Array.h>

// An object that provides an interface to a rendering algorithm, which operates on render nodes.
// It renders nodes from the passed one to the first not suitable for it, and returns that unprocessed
// node as a new head. In case of a render tree, renderer must return node of the same level to that a
// passed node belongs. Therefore, if renderer accepts a render tree node, it must accept all its children.

namespace Render
{
struct CRenderNode;
class CGPUDriver;

class IRenderer: public Core::CRTTIBaseClass
{
	__DeclareClassNoFactory;

public:

	virtual CArray<CRenderNode>::CIterator Render(CGPUDriver& GPU, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr) = 0;
};

}

#endif
