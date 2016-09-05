#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_H__
#define __DEM_L1_RENDER_RENDERER_H__

#include <Core/RTTIBaseClass.h>
#include <Data/Array.h>
#include <Math/Matrix44.h>

// An object that provides an interface to a rendering algorithm, which operates on render nodes.
// It renders nodes from the passed one to the first not suitable for it, and returns that unprocessed
// node as a new head. In case of a render tree, renderer must return node of the same level as a
// passed node. Therefore, if renderer accepts a render tree node, it must accept all its children.

namespace Render
{
struct CRenderNode;
struct CRenderNodeContext;
class CGPUDriver;

class IRenderer: public Core::CRTTIBaseClass
{
	__DeclareClassNoFactory;

public:

	struct CRenderContext
	{
		Render::CGPUDriver* pGPU;
		vector3				CameraPosition;
		matrix44			ViewProjection;
	};

	virtual bool							PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context) = 0;
	virtual CArray<CRenderNode*>::CIterator	Render(const CRenderContext& Context, CArray<CRenderNode*>& RenderQueue, CArray<CRenderNode*>::CIterator ItCurr) = 0;
};

}

#endif
