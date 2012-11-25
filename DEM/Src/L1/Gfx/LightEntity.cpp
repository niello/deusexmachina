#include "LightEntity.h"

#include <Gfx/GfxServer.h>
#include <scene/nlightnode.h>
#include <mathlib/sphere.h>

namespace Graphics
{
ImplementRTTI(Graphics::CLightEntity, Graphics::CRenderableEntity);
ImplementFactory(Graphics::CLightEntity);

int CLightEntity::LightUID = 0;

void CLightEntity::UpdateNebulaLight()
{
	if (!refLightNode.isvalid()) return;
	nLightNode* pLightNode = refLightNode;
	if (Light.GetType() == nLight::Directional) Light.SetRange(100000.0f);
	pLightNode->Light = Light;
	float Range = Light.GetRange();
	pLightNode->SetLocalBox(bbox3(vector3(0.0f, 0.0f, 0.0f), vector3(Range, Range, Range)));
}
//---------------------------------------------------------------------

void CLightEntity::Activate()
{
	n_assert(!Flags.Is(ACTIVE));
	n_assert(!Resource.IsLoaded());
	n_assert(!refLightNode.isvalid());

	nString Name = "lights/light";
	Name.AppendInt(LightUID++);

	nKernelServer::Instance()->PushCwd(GfxSrv->GetGfxRoot());
	refLightNode = (nLightNode*)nKernelServer::Instance()->New("nlightnode", Name.Get());
	nKernelServer::Instance()->PopCwd();
	UpdateNebulaLight();
	RenderCtx.SetFlag(nRenderContext::CastShadows, Light.GetCastShadows());

	SetResourceName(Name.Get());

	CRenderableEntity::Activate();
}
//---------------------------------------------------------------------

void CLightEntity::Deactivate()
{
	n_assert(refLightNode.isvalid());
	CRenderableEntity::Deactivate();
	refLightNode->Release();
	refLightNode.invalidate();
}
//---------------------------------------------------------------------

void CLightEntity::Render()
{
	if (!GetVisible()) return;
	ValidateResource();
	UpdateRenderContextVariables();
	RenderCtx.SetGlobalBox(GetBox());
	RenderRenderContext(RenderCtx);
}
//---------------------------------------------------------------------

EClipStatus CLightEntity::GetBoxClipStatus(const bbox3& Box)
{
	if (nLight::Directional == Light.GetType()) return Inside;
	else if (nLight::Point == Light.GetType())
	{
		sphere Sphere(Transform.pos_component(), Light.GetRange());
		return Sphere.clipstatus(Box);
	}
	else
	{
		n_error("CLightEntity::GetBoxClipStatus() Invalid Light type!");
		return Outside;
	}
}
//---------------------------------------------------------------------

} // namespace Graphics
