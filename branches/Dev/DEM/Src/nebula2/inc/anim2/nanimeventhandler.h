#ifndef N_ANIMEVENTHANDLER_H
#define N_ANIMEVENTHANDLER_H
//------------------------------------------------------------------------------
/**
    @class nAnimEventHandler
    @ingroup Anim2
    @brief User-derivable anim event handler class. Derive your own subclass
    and register it with an anim state array object to listen to
    animation events.

    (C) 2005 Radon Labs GmbH
*/
#include "util/nnode.h"
#include "anim2/nanimeventtrack.h"
#include <Core/RefCounted.h>
#include "kernel/nRefCounted.h"

//------------------------------------------------------------------------------
class nAnimEventHandler: public nRefCounted //Core::CRefCounted
{
public:
    /// constructor
    nAnimEventHandler();
    /// destructor
    virtual ~nAnimEventHandler();
    /// handle an animation event
    virtual void HandleEvent(const nAnimEventTrack& track, int eventIndex);
};
//------------------------------------------------------------------------------
#endif
