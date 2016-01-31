#pragma once
#ifndef __DEM_L1_FRAME_MODEL_RENDERER_H__
#define __DEM_L1_FRAME_MODEL_RENDERER_H__

#include <Frame/Renderer.h>

// Default renderer for CModel render objects

namespace Frame
{

class CModelRenderer: public IRenderer
{
	__DeclareClass(CModelRenderer);

public:

	virtual CArray<CRenderNode>::CIterator Render(CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr);
};

}

#endif
