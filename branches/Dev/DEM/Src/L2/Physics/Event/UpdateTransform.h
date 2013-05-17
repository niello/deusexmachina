#pragma once
#ifndef __DEM_L2_NEVENT_UPDATE_TF_H__
#define __DEM_L2_NEVENT_UPDATE_TF_H__

#include <Events/EventNative.h>

// Updates the transform of a entity, does not set the physics transform. All propertise
// that need to update when entity transformation changes need listen to this messages.
// To set the transformation of a entity (including the physics) use the SetTransform message.

namespace Event
{

class UpdateTransform: public Events::CEventNative
{
    __DeclareClass(UpdateTransform);

public:

	matrix44 Transform;
    bool     Smoothing;

	UpdateTransform(): Smoothing(true) {}
};

}

#endif
