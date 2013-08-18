#pragma once
#ifndef __CIDE_PROP_EDITOR_CAMERA_H__
#define __CIDE_PROP_EDITOR_CAMERA_H__

#include <Scene/Camera.h>
#include <Math/Polar.h>
#include <Events/EventsFwd.h>

// Specialized editor camera

namespace Scene
{

class CPropEditorCamera: public CCamera
{
	__DeclareClass(CPropEditorCamera);

private:

	CPolar		Angles;
	float		Distance;
	vector3		COI;				// Center of interest
	bool		UpdateFromInside;

	void			SetupFromTransform();

	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);

	virtual void	OnObtainCameraFocus();
	virtual void	OnRender();

public:

	CPropEditorCamera(): UpdateFromInside(false) {}

	virtual void Activate();
	virtual void Deactivate();
};

}

#endif
