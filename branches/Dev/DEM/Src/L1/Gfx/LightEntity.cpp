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
	pLightNode->SetType(Light.GetType());
	pLightNode->SetCastShadows(Light.GetCastShadows());
	pLightNode->SetVector(nShaderState::LightDiffuse, Light.GetDiffuse());
	pLightNode->SetVector(nShaderState::LightSpecular, Light.GetSpecular());
	pLightNode->SetVector(nShaderState::LightAmbient, Light.GetAmbient());
	float Range = (nLight::Directional == Light.GetType()) ? 100000.0f : Light.GetRange();
	pLightNode->SetLocalBox(bbox3(vector3(0.0f, 0.0f, 0.0f), vector3(Range, Range, Range)));
	pLightNode->SetFloat(nShaderState::LightRange, Range);
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
		switch (Sphere.clipstatus(Box))
		{
			case sphere::Inside:	return Inside;
			case sphere::Clipped:	return Clipped;
			case sphere::Outside:
			default:				return Outside;
		}
	}
	else
	{
		n_error("CLightEntity::GetBoxClipStatus() Invalid Light type!");
		return Outside;
	}
}
//---------------------------------------------------------------------

} // namespace Graphics
