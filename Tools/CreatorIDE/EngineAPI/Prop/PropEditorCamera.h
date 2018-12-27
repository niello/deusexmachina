#pragma once
#ifndef __CIDE_PROP_EDITOR_CAMERA_H__
#define __CIDE_PROP_EDITOR_CAMERA_H__

#include <Camera/Prop/PropCamera.h>
#include <mathlib/polar.h>

// Specialized editor camera

namespace Properties
{

class CPropEditorCamera: public CPropCamera
{
	DeclareRTTI;
	DeclareFactory(CPropEditorCamera);

private:

	polar2	Angles;
	float	Distance;
	vector3	COI;				// Center of interest
	bool	UpdateFromInside;

	void SetupFromTransform();

	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);

	virtual void OnObtainCameraFocus();
	virtual void OnRender();

public:

	CPropEditorCamera(): UpdateFromInside(false) {}

	virtual void Activate();
	virtual void Deactivate();
};

RegisterFactory(CPropEditorCamera);

}

#endif
