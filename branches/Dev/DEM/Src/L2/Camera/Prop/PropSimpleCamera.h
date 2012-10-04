#pragma once
#ifndef __DEM_L2_PROP_SIMPLE_CAMERA_H__
#define __DEM_L2_PROP_SIMPLE_CAMERA_H__

#include "PropCamera.h"

// A simple camera, moves with WASD and can look around.
// Based on mangalore SimpleCameraProperty (C) 2006 Radon Labs GmbH

namespace Properties
{

using namespace Events;

class CPropSimpleCamera: public CPropCamera
{
	DeclareRTTI;
	DeclareFactory(CPropSimpleCamera);

protected:

	vector3	RelMove;
	float	RelHorizontalRotation;
	float	RelVerticalRotation;
	float	RelHorizontalRotationFactor;
	float	RelVerticalRotationFactor;
	float	RelMoveFactor;

	DECLARE_EVENT_HANDLER(MoveByOffset, OnMoveByOffset);
	DECLARE_EVENT_HANDLER(CameraOrbit, OnCameraOrbit);
	
	virtual void OnRender();

public:

	CPropSimpleCamera();
	
	virtual void Activate();
	virtual void Deactivate();
};

RegisterFactory(CPropSimpleCamera);

}

#endif