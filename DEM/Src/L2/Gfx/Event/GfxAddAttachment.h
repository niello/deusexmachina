#pragma once
#ifndef __DEM_L2_NEVENT_GFX_ADD_ATTACH_H__
#define __DEM_L2_NEVENT_GFX_ADD_ATTACH_H__

#include <Events/EventNative.h>

// Attach a graphics entity defined by a resource name to a joint.

namespace Graphics
{
	class CShapeEntity;
}

namespace Event
{

class GfxAddAttachment: public Events::CEventNative
{
	DeclareRTTI;
	DeclareFactory(GfxAddAttachment);

public:

	nString					JointName;
	nString					GfxResourceName;
	Graphics::CShapeEntity*	GfxEntity;
	matrix44				OffsetMatrix;

	GfxAddAttachment(): GfxEntity(NULL) {}
};

RegisterFactory(GfxAddAttachment);

}

#endif
