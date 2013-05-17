#pragma once
#ifndef __DEM_L2_NEVENT_CAMERA_ORBIT_H__
#define __DEM_L2_NEVENT_CAMERA_ORBIT_H__

#include <Events/EventNative.h>

// A camera orbit rotation message.

namespace Event
{

class CameraOrbit: public Events::CEventNative
{
    __DeclareClass(CameraOrbit);

public:

	float	AngleHoriz;
	float	AngleVert;
	int		DirHoriz;
	int		DirVert;

	CameraOrbit(): AngleHoriz(0.f), AngleVert(0.f), DirHoriz(0), DirVert(0) { }
	CameraOrbit(float AngleH, float AngleV): AngleHoriz(AngleH), AngleVert(AngleV), DirHoriz(0), DirVert(0) { }
};

}

#endif
