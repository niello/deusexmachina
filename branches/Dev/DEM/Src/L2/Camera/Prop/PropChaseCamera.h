#pragma once
#ifndef __DEM_L2_PROP_CHASE_CAMERA_H__
#define __DEM_L2_PROP_CHASE_CAMERA_H__

// A chase camera for 3rd person camera control.
// Based on mangalore CPropChaseCamera (C) 2005 Radon Labs GmbH

#include "PropCamera.h"
#include <Scene/AnimControllerThirdPerson.h>
#include <Scene/Camera.h>
#include <mathlib/polar.h>
#include <util/npfeedbackloop.h>

namespace Prop
{

using namespace Events;

class CPropChaseCamera: public CPropCamera
{
	__DeclareClass(CPropChaseCamera);

protected:

	polar2								Angles;
	float								Distance;
	nPFeedbackLoop<vector3>				Position;
	nPFeedbackLoop<vector3>				Lookat;

	Scene::PSceneNode					Node;
	Scene::PCamera						Camera;
	Scene::PAnimControllerThirdPerson	Ctlr;

	virtual vector3 DoCollideCheck(const vector3& from, const vector3& to);

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(CameraReset, OnCameraReset);
	DECLARE_EVENT_HANDLER(CameraOrbit, OnCameraOrbit);
	DECLARE_EVENT_HANDLER(CameraDistance, OnCameraDistanceChange);

	virtual void UpdateCamera();
	virtual void ResetCamera();

	virtual void OnRender();
	virtual void OnObtainCameraFocus();

public:

	CPropChaseCamera(): Distance(0.0f) {}

	virtual void Activate();
	virtual void Deactivate();
};

} // namespace Prop

#endif
