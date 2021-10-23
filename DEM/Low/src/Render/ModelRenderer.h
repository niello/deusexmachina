#pragma once
#include <Render/Renderer.h>
#include <Render/VertexComponent.h>
#include <Data/FixedArray.h>
#include <Data/Ptr.h>
#include <map>

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

	CFixedArray<CVertexComponent>           InstanceDataDecl;
	std::map<CVertexLayout*, PVertexLayout> InstancedLayouts;	//!!!duplicate in different instances of the same renderer!
	PVertexBuffer                           InstanceVB;        //!!!binds an RP to a specific GPU!
	UPTR                                    InstanceVBSize = 0;

public:

	virtual bool							Init(bool LightingEnabled, const Data::CParams& Params) override;
	virtual bool							PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context) override;
	virtual CRenderQueueIterator Render(const CRenderContext& Context, CRenderQueue& RenderQueue, CRenderQueueIterator ItCurr) override;
};

}
