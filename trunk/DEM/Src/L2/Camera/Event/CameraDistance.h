#pragma once
#ifndef __DEM_L2_NEVENT_CAMERA_DISTANCE_H__
#define __DEM_L2_NEVENT_CAMERA_DISTANCE_H__

#include <Events/EventNative.h>

// Change the distance of a 3rd person camera to its lookat point.

namespace Event
{

class CameraDistance: public Events::CEventNative
{
    DeclareRTTI;
    DeclareFactory(CameraDistance);

public:

	float RelChange;

	CameraDistance(): RelChange(0.f) {}
	CameraDistance(float _RelChange): RelChange(_RelChange) {}
};

RegisterFactory(CameraDistance);

}

#endif

