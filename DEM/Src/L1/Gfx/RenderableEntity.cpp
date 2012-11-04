#include "RenderableEntity.h"

#include <Gfx/GfxServer.h>
#include <variable/nvariableserver.h>
#include <scene/nsceneserver.h>
#include <scene/ntransformnode.h>

namespace Graphics
{
ImplementRTTI(Graphics::CRenderableEntity, Core::CRefCounted);

CRenderableEntity::CRenderableEntity():
	TimeVarHandle(nVariableServer::Instance()->GetVariableHandleByName("time")),
	OneVarHandle(nVariableServer::Instance()->GetVariableHandleByName("one")),
	WindVarHandle(nVariableServer::Instance()->GetVariableHandleByName("wind"))
{
	nFloat4 Wind = { 1.0f, 0.0f, 0.0f, 0.5f };
	RenderCtx.AddVariable(nVariable(TimeVarHandle, 0.0f));
	RenderCtx.AddVariable(nVariable(OneVarHandle, 1.0f));
	RenderCtx.AddVariable(nVariable(WindVarHandle, Wind));
}
//---------------------------------------------------------------------

// This method is called when the graphics object is attached to a game entity.
void CRenderableEntity::Activate()
{
	n_assert(!Resource.IsLoaded());
	ValidateResource();
	CEntity::Activate();
}
//---------------------------------------------------------------------

void CRenderableEntity::Deactivate()
{
	CEntity::Deactivate();

	if (Resource.IsLoaded())
	{
		nTransformNode* pNode = Resource.GetNode();
		n_assert(pNode);
		pNode->RenderContextDestroyed(&RenderCtx);
		Resource.Unload();
	}
}
//---------------------------------------------------------------------

void CRenderableEntity::ValidateResource()
{
	if (!Resource.IsLoaded())
	{
		Resource.Load();
		nTransformNode* pNode = Resource.GetNode();
		n_assert(pNode);
		RenderCtx.SetRootNode(pNode);
		pNode->RenderContextCreated(&RenderCtx);
		SetLocalBox(pNode->GetLocalBox());
	}
}
//---------------------------------------------------------------------

void CRenderableEntity::UpdateGlobalBox()
{
	n_assert(Flags.Is(GLOBAL_BOX_DIRTY));
	Flags.Clear(GLOBAL_BOX_DIRTY);
	GlobalBBox = LocalBBox;
	GlobalBBox.transform(Resource.Name.IsValid() ? Resource.GetNode()->GetTransform() * Transform : Transform);
}
//---------------------------------------------------------------------

void CRenderableEntity::UpdateRenderContextVariables()
{
	RenderCtx.SetTransform(Transform);
	RenderCtx.GetVariable(TimeVarHandle)->SetFloat((float)GetEntityTime());
	RenderCtx.SetFrameId(GfxSrv->GetFrameID());
}
//---------------------------------------------------------------------

void CRenderableEntity::OnRenderBefore()
{
}
//---------------------------------------------------------------------

void CRenderableEntity::RenderRenderContext(nRenderContext& Ctx)
{
	nArray<PEntity>& LightLinks = Links[LightLink];

	Ctx.ClearLinks();
	for (int i = 0; i < LightLinks.Size(); i++)
		Ctx.AddLink(&((CRenderableEntity*)LightLinks[i].get_unsafe())->RenderCtx);

	nSceneServer::Instance()->Attach(&Ctx);
}
//---------------------------------------------------------------------

} // namespace Graphics
