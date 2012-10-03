#pragma once
#ifndef __DEM_L2_NEVENT_SET_TF_H__
#define __DEM_L2_NEVENT_SET_TF_H__

#include <Events/EventNative.h>

// Set the complete transform of a entity, including the  physics tranform.
// Use not per Frame, the physics won't like it!

namespace Event
{

class SetTransform: public Events::CEventNative
{
	DeclareRTTI;
	DeclareFactory(SetTransform);

public:

	matrix44 Transform;

	SetTransform() {}
	SetTransform(const matrix44& Tfm): Transform(Tfm) {}
};

RegisterFactory(SetTransform);

}

#endif
