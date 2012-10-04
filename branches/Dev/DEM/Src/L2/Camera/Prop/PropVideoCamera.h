#pragma once
#ifndef __DEM_L2_PROP_VIDEO_CAMERA_H__
#define __DEM_L2_PROP_VIDEO_CAMERA_H__

#include "PropCamera.h"
//#include <tools/nmayacamcontrol.h>

// A manually controlled camera property which implements different control models.
// Based on mangalore VideoCameraProperty_(C) 2005 Radon Labs GmbH

namespace Attr
{
	DeclareVector3(VideoCameraCenterOfInterest);
	DeclareVector3(VideoCameraDefaultUpVec);
};

namespace Properties
{

class CPropVideoCamera: public CPropCamera
{
	DeclareRTTI;
	DeclareFactory(CPropVideoCamera);

private:

	//nMayaCamControl MayaCamera;

	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);
	DECLARE_EVENT_HANDLER(OnObtainInputFocus, OnObtainInputFocus);
	DECLARE_EVENT_HANDLER(OnLoseInputFocus, OnLoseInputFocus);

	virtual void OnRender();

public:

	virtual void Activate();
	virtual void Deactivate();
	virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);
};

RegisterFactory(CPropVideoCamera);

}

#endif
