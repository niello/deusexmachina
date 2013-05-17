#pragma once
#ifndef __DEM_L2_PROP_CAMERA_H__
#define __DEM_L2_PROP_CAMERA_H__

#include <Game/Property.h>
#include <DB/AttrID.h>
//#include <vfx/shakeeffecthelper.h>
#include <Game/Entity.h>

/**
    A camera property adds the ability to manipulate the camera to an entity.
    Please note that more advanced camera properties should always be
    derived from the class camera property if camera focus handling is desired,
    since the CFocusManager will only work on game entities which have
    a CPropCamera (or a subclass) attached.

    It is completely ok though to handle camera manipulation in a property
    not derived from CPropCamera, but please be aware that the
    CFocusManager will ignore those.

    The camera property will generally

    Based on mangalore CameraProperty (C) 2005 Radon Labs GmbH
*/

namespace Attr
{
	DeclareFloat(FieldOfView);
};

namespace Properties
{

class CPropCamera: public Game::CProperty
{
	__DeclareClass(CPropCamera);
	__DeclarePropertyStorage;

protected:

	//VFX::ShakeEffectHelper ShakeFxHelper;

	DECLARE_EVENT_HANDLER_VIRTUAL(OnObtainCameraFocus, OnObtainCameraFocus);
	DECLARE_EVENT_HANDLER_VIRTUAL(OnLoseCameraFocus, OnLoseCameraFocus);
	DECLARE_EVENT_HANDLER_VIRTUAL(OnRender, OnRender);

public:

	CPropCamera();
	virtual ~CPropCamera() {}

	virtual void Activate();
	virtual void Deactivate();

	bool HasFocus() const;
};

}

#endif
