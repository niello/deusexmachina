#pragma once
#include <Render/Renderer.h>

// Default renderer for CSkybox render objects.

namespace Render
{
class CSkybox;

class CSkyboxRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

public:

	CSkyboxRenderer();

	virtual bool							Init(bool LightingEnabled) { OK; }
	virtual bool							PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context);
	virtual CArray<CRenderNode*>::CIterator	Render(const CRenderContext& Context, CArray<CRenderNode*>& RenderQueue, CArray<CRenderNode*>::CIterator ItCurr);
};

}
