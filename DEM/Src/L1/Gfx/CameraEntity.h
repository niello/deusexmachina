#pragma once
#ifndef __DEM_L2_GFX_CAMERA_ENTITY_H__
#define __DEM_L2_GFX_CAMERA_ENTITY_H__

#include "Entity.h"
#include <gfx2/ncamera2.h>

// A Camera entity can check shape entities for visibility and
// establishes a link between visible shape entities and the Camera entity.
// A Camera can be made the current Camera in the level it is attached to.
// Only one Camera entity can be current at any time.

namespace Graphics
{

class CCameraEntity: public CEntity
{
	DeclareRTTI;
	DeclareFactory(CCameraEntity);

private:

	nCamera2	Camera;
	matrix44	ViewProjMatrix;
	matrix44	ViewMatrix;
	bool		ViewProjDirty;

	void UpdateViewProjection();

public:

	CCameraEntity();
	virtual ~CCameraEntity() {}

	virtual void		Activate();
	virtual void		OnRenderBefore();
	virtual void		RenderLinks();

	virtual EEntityType	GetType() const { return GFXCamera; }
	virtual void		SetTransform(const matrix44& Tfm);
	virtual EClipStatus	GetBoxClipStatus(const bbox3& Box);
	virtual EClipStatus	GetViewVolumeClipStatus(const matrix44& Tfm, nCamera2& OtherCamera);
	void				SetCamera(const nCamera2& New);
	nCamera2&			GetCamera() { return Camera; }
	const matrix44&		GetViewProjection();
	const matrix44&		GetView();
	
	// For CLinkedListSet
	virtual EEntityType	GetKey() const { return GFXCamera; }
};
//---------------------------------------------------------------------

RegisterFactory(CCameraEntity);

inline void CCameraEntity::UpdateViewProjection()
{
	n_assert(ViewProjDirty);
	ViewProjDirty = false;
	ViewMatrix = GetTransform();
	ViewMatrix.invert_simple();
	ViewProjMatrix = ViewMatrix;
	ViewProjMatrix *= Camera.GetProjection();
}
//---------------------------------------------------------------------

inline void CCameraEntity::SetTransform(const matrix44& Tfm)
{
	ViewProjDirty = true;
	CEntity::SetTransform(Tfm);
}
//---------------------------------------------------------------------

inline const matrix44& CCameraEntity::GetViewProjection()
{
	if (ViewProjDirty) UpdateViewProjection();
	return ViewProjMatrix;
}
//---------------------------------------------------------------------

inline const matrix44& CCameraEntity::GetView()
{
	if (ViewProjDirty) UpdateViewProjection();
	return ViewMatrix;
}
//---------------------------------------------------------------------

}

#endif
