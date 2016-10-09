#pragma once
#ifndef __DEM_L1_RENDER_SKYBOX_RENDERER_H__
#define __DEM_L1_RENDER_SKYBOX_RENDERER_H__

#include <Render/Renderer.h>

// Default renderer for CSkybox render objects.

namespace Render
{
class CSkybox;

class CSkyboxRenderer: public IRenderer
{
	__DeclareClass(CSkyboxRenderer);

protected:

	UPTR InputSet_Skybox;

public:

	CSkyboxRenderer();

	virtual bool							Init(bool LightingEnabled) { OK; }
	virtual bool							PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context);
	virtual CArray<CRenderNode*>::CIterator	Render(const CRenderContext& Context, CArray<CRenderNode*>& RenderQueue, CArray<CRenderNode*>::CIterator ItCurr);
};

}

#endif
