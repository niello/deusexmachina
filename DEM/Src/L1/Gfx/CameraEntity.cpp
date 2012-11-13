#include "CameraEntity.h"

#include <Gfx/Level.h>
#include <Gfx/RenderableEntity.h>
#include <gfx2/ngfxserver2.h>

namespace Graphics
{
ImplementRTTI(Graphics::CCameraEntity, Graphics::CEntity);
ImplementFactory(Graphics::CCameraEntity);

CCameraEntity::CCameraEntity(): Camera(60.0f, 4.0f / 3.0f, 0.1f, 2500.0f), ViewProjDirty(false)
{
}
//---------------------------------------------------------------------

// Since this is a Camera there is no graphics resource attached which would normally
// deliver the bounding box.
void CCameraEntity::Activate()
{
	CEntity::Activate();
	SetLocalBox(Camera.GetBox());
}
//---------------------------------------------------------------------

EClipStatus CCameraEntity::GetBoxClipStatus(const bbox3& Box)
{
	if (ViewProjDirty) UpdateViewProjection();
	return Box.clipstatus(ViewProjMatrix);
}
//---------------------------------------------------------------------

// Get clip status of other view volume against this entity:
EClipStatus CCameraEntity::GetViewVolumeClipStatus(const matrix44& Tfm, nCamera2& OtherCamera)
{
	if (ViewProjDirty) UpdateViewProjection();
	return OtherCamera.GetClipStatus(Tfm, ViewProjMatrix);
}
//---------------------------------------------------------------------

// Change Camera characteristics. Note that this also updates the entity's bounding box
// and may change the entity's position the cell tree.
void CCameraEntity::SetCamera(const nCamera2& New)
{
	ViewProjDirty = true;
	Camera = New;
	SetLocalBox(Camera.GetBox());
	if (pLevel && pQTNode) pLevel->UpdateEntityLocation(this);
}
//---------------------------------------------------------------------

// Distribute to entities visible by camera
void CCameraEntity::OnRenderBefore()
{
	nGfxServer2::Instance()->SetCamera(Camera);

	nArray<PEntity>& CamLinks = Links[CameraLink];
	for (int i = 0; i < CamLinks.Size(); i++)
	{
		//!!!bad cast! check can camera be in links and if no, change link array base type to CRenderableEntity*!
		CRenderableEntity* pLink = (CRenderableEntity*)CamLinks[i].get();
		n_assert2(!pLink->IsA(CCameraEntity::RTTI), "Camera link detected!");
		n_assert(CamLinks[i].get() != this);
		pLink->OnRenderBefore();
	}
}
//---------------------------------------------------------------------

void CCameraEntity::RenderLinks()
{
	nArray<PEntity>& CamLinks = Links[CameraLink];
	for (int i = 0; i < CamLinks.Size(); i++)
	{
		//!!!bad cast! check can camera be in links and if no, change link array base type to CRenderableEntity*!
		CRenderableEntity* pLink = (CRenderableEntity*)CamLinks[i].get();
		n_assert(CamLinks[i].get() != this && pLink->GetVisible());
		pLink->Render();
	}
}
//---------------------------------------------------------------------

} // namespace Graphics
