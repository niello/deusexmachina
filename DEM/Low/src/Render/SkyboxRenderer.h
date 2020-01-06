#pragma once
#include <Render/Renderer.h>

// Default renderer for CSkybox render objects.

namespace Render
{

class CSkyboxRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

public:

	CSkyboxRenderer();

	virtual bool                 Init(bool LightingEnabled) { OK; }
	virtual bool                 PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context);
	virtual CRenderQueueIterator Render(const CRenderContext& Context, CRenderQueue& RenderQueue, CRenderQueueIterator ItCurr);
};

}
