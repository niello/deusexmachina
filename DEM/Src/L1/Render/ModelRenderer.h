#pragma once
#ifndef __DEM_L1_FRAME_MODEL_RENDERER_H__
#define __DEM_L1_FRAME_MODEL_RENDERER_H__

#include <Render/Renderer.h>

// Default renderer for CModel render objects

namespace Render
{

class CModelRenderer: public IRenderer
{
	__DeclareClass(CModelRenderer);

protected:

	UPTR	InputSet_Model;
	UPTR	InputSet_ModelSkinned;
	UPTR	InputSet_ModelInstanced;

public:

	CModelRenderer();

	virtual void							PrepareNode(CRenderNode& Node);
	virtual CArray<CRenderNode>::CIterator	Render(CGPUDriver& GPU, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr);
};

}

#endif
