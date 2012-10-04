#pragma once
#ifndef __DEM_L2_NEVENT_GFX_REM_ATTACH_H__
#define __DEM_L2_NEVENT_GFX_REM_ATTACH_H__

#include <Events/EventNative.h>

// Remove an attachment from a joint.

namespace Event
{

class GfxRemoveAttachment: public Events::CEventNative
{
    DeclareRTTI;
    DeclareFactory(GfxRemoveAttachment);

public:

	nString JointName;
};

RegisterFactory(GfxRemoveAttachment);

}

#endif
