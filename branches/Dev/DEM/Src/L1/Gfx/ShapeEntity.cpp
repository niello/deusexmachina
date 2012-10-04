#include "ShapeEntity.h"

#include <Gfx/GfxServer.h>
#include <Gfx/LightEntity.h>
#include <scene/ntransformnode.h>
#include <gfx2/ngfxserver2.h>

namespace Graphics
{
ImplementRTTI(Graphics::CShapeEntity, Graphics::CRenderableEntity);
ImplementFactory(Graphics::CShapeEntity);

CShapeEntity::CShapeEntity(): ShadowBoxFrameID(0)
{
	nFloat4 Wind = { 1.0f, 0.0f, 0.0f, 0.5f };
	ShadowRenderCtx.AddVariable(nVariable(TimeVarHandle, 0.0f));
	ShadowRenderCtx.AddVariable(nVariable(OneVarHandle, 1.0f));
	ShadowRenderCtx.AddVariable(nVariable(WindVarHandle, Wind));
}
//---------------------------------------------------------------------

// This method is called when the graphics object is attached to a game entity.
void CShapeEntity::Activate()
{
	n_assert(!ShadowResource.IsLoaded());
	CRenderableEntity::Activate();
	ValidateShadowResource();
}
//---------------------------------------------------------------------

void CShapeEntity::Deactivate()
{
	if (ShadowResource.IsLoaded())
	{
		nTransformNode* pNode = ShadowResource.GetNode();
		n_assert(pNode);
		pNode->RenderContextDestroyed(&ShadowRenderCtx);
		ShadowResource.Unload();
	}

	CRenderableEntity::Deactivate();
}
//---------------------------------------------------------------------

void CShapeEntity::ValidateShadowResource()
{
	if (ShadowResource.Name.IsValid() && !ShadowResource.IsLoaded())
	{
		ShadowResource.Load();
		nTransformNode* pNode = ShadowResource.GetNode();
		n_assert(pNode);
		ShadowRenderCtx.SetRootNode(pNode);
		pNode->RenderContextCreated(&ShadowRenderCtx);
	}
}
//---------------------------------------------------------------------

void CShapeEntity::UpdateRenderContextVariables()
{
	float Time = (float)GetEntityTime();
	RenderCtx.SetTransform(Transform);
	RenderCtx.GetVariable(TimeVarHandle)->SetFloat(Time);
	if (ShadowResource.IsLoaded())
	{
		ShadowRenderCtx.SetTransform(Transform);
		ShadowRenderCtx.GetVariable(TimeVarHandle)->SetFloat(Time);
	}
}
//---------------------------------------------------------------------

// Render the graphics pEntity. This attaches all resource objects of the graphics entity to
// the scene. The method should only be called if the graphics object is actually visible!
void CShapeEntity::Render()
{
	if (!GetVisible()) return;

	ValidateResource();
	UpdateRenderContextVariables();
	RenderCtx.SetFrameId(GfxSrv->GetFrameID());

	RenderCtx.SetGlobalBox(GetShadowBox());
	RenderRenderContext(RenderCtx);

	if (ShadowResource.IsLoaded()) //???if (ShadowRenderCtx.IsValid())?
	{
		ShadowRenderCtx.SetGlobalBox(RenderCtx.GetGlobalBox());
		RenderRenderContext(ShadowRenderCtx);
	}
}
//---------------------------------------------------------------------

void CShapeEntity::RenderDebug()
{
	nGfxServer2::Instance()->DrawShape(nGfxServer2::Box, ShadowBBox.to_matrix44(), pQTNode->Data.DebugColor);
}
//---------------------------------------------------------------------

const bbox3& CShapeEntity::GetShadowBox()
{
	uint CurrFrameID = GfxSrv->GetFrameID();
	if (CurrFrameID != ShadowBoxFrameID)
	{
		ShadowBoxFrameID = CurrFrameID;
		ShadowBBox = GetBox();

		const float DirExtrudeLength = ShadowBBox.size().y;
		const vector3& Pos = GetTransform().pos_component();

		vector3 LightVector, ExtrudeVector;
		nArray<PEntity>& LightLinks = Links[LightLink];
		for (int i = 0; i < LightLinks.Size(); i++)
		{
			CLightEntity* pLightEntity = (CLightEntity*)LightLinks[i].get();
			n_assert(pLightEntity->IsA(CLightEntity::RTTI));
			if (pLightEntity->Light.GetCastShadows())
			{
				const nLight& Light = pLightEntity->Light;
				nLight::Type LightType = Light.GetType();
				if (nLight::Directional == LightType)
				{
					LightVector = pLightEntity->GetTransform().z_component();
					LightVector.norm();
					ExtrudeVector = LightVector * DirExtrudeLength;
					ShadowBBox.extend(ShadowBBox.vmin - ExtrudeVector);
					ShadowBBox.extend(ShadowBBox.vmax - ExtrudeVector);
				}
				else if (nLight::Point == LightType)
				{
					const vector3& LightPos = pLightEntity->GetTransform().pos_component();
					LightVector = Pos - LightPos;
					LightVector.norm();
					ExtrudeVector = LightVector * Light.GetRange();
					ShadowBBox.extend(LightPos + ExtrudeVector);
				}
				else
				{
					// FIXME: unsupported Light type
					n_error("IMPLEMENT ME!!!");
				}
			}
		}
	}
	return ShadowBBox;
}
//---------------------------------------------------------------------

} // namespace Graphics
