#pragma once
#ifndef __DEM_L2_PROP_VIDEO_CAMERA_2_H__
#define __DEM_L2_PROP_VIDEO_CAMERA_2_H__

#include "PropCamera.h"
//#include <tools/nmayacamcontrol.h>
#include <DB/AttrID.h>

// A manually controlled camera property which implements different control models.
// Based on mangalore VideoCameraProperty2_(C) 2005 Radon Labs GmbH

namespace Attr
{
	DeclareFloat(FilmAspectRatio);
	DeclareFloat(NearClipPlane);
	DeclareFloat(FarClipPlane);
	DeclareMatrix44(ProjectionMatrix);
};

namespace Properties
{

class CPropVideoCamera2 : public CPropCamera
{
	DeclareRTTI;
	DeclareFactory(CPropVideoCamera2);

private:

	//nMayaCamControl MayaCamera;

	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);
	DECLARE_EVENT_HANDLER(OnObtainInputFocus, OnObtainInputFocus);
	DECLARE_EVENT_HANDLER(OnLoseInputFocus, OnLoseInputFocus);

	virtual void OnRender();
	virtual void OnObtainCameraFocus();

public:

	virtual void Activate();
	virtual void Deactivate();
	virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);
};

RegisterFactory(CPropVideoCamera2);

}

#endif
