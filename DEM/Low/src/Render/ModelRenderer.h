#pragma once
#ifndef __DEM_L1_RENDER_MODEL_RENDERER_H__
#define __DEM_L1_RENDER_MODEL_RENDERER_H__

#include <Render/Renderer.h>
#include <Render/VertexComponent.h>
#include <Data/FixedArray.h>
#include <Data/Dictionary.h>
#include <Data/Ptr.h>

// Default renderer for CModel render objects

namespace Render
{
typedef Ptr<class CVertexLayout> PVertexLayout;
typedef Ptr<class CVertexBuffer> PVertexBuffer;

class CModelRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

protected:

	static const U32						INSTANCE_BUFFER_STREAM_INDEX = 1;
	static const U16						MAX_LIGHT_COUNT_PER_OBJECT = 8; //???to setting?

	CFixedArray<CVertexComponent>			InstanceDataDecl;
	CDict<CVertexLayout*, PVertexLayout>	InstancedLayouts;	//!!!duplicate in different instances of the same renderer!
	PVertexBuffer							InstanceVB;			//!!!binds an RP to a specific GPU!
	UPTR									MaxInstanceCount;	//???where to define? in a phase? or some setting? or move to CView with a VB?

public:

	virtual bool							Init(bool LightingEnabled);
	virtual bool							PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context);
	virtual CRenderQueueIterator Render(const CRenderContext& Context, CRenderQueue& RenderQueue, CRenderQueueIterator ItCurr);
};

}

#endif
