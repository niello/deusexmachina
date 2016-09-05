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
	__DeclareClass(CModelRenderer);

protected:

	const U32								INSTANCE_BUFFER_STREAM_INDEX = 1;

	UPTR									InputSet_Model;
	UPTR									InputSet_ModelSkinned;
	UPTR									InputSet_ModelInstanced;

	CFixedArray<CVertexComponent>			InstanceDataDecl;
	CDict<CVertexLayout*, PVertexLayout>	InstancedLayouts;	//!!!duplicate in different instances of the same renderer!
	PVertexBuffer							InstanceVB;			//!!!binds an RP to a specific GPU!
	UPTR									MaxInstanceCount;	//???where to define? in a phase? or some setting? or move to CView with a VB?

public:

	CModelRenderer();

	virtual bool							PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context);
	virtual CArray<CRenderNode*>::CIterator	Render(const CRenderContext& Context, CArray<CRenderNode*>& RenderQueue, CArray<CRenderNode*>::CIterator ItCurr);
};

}

#endif
