//------------------------------------------------------------------------------
//  nanimationserver_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "anim2/nanimationserver.h"
#include "resource/nresourceserver.h"
#include <kernel/nkernelserver.h>

nAnimationServer* nAnimationServer::Singleton = 0;

//------------------------------------------------------------------------------
/**
*/
nAnimationServer::nAnimationServer()
{
    n_assert(0 == Singleton);
    Singleton = this;
}

//------------------------------------------------------------------------------
/**
*/
nAnimationServer::~nAnimationServer()
{
    // unload all animation resources
    if (nResourceServer::HasInstance())
        nResourceServer::Instance()->UnloadResources(nResource::Animation);
    n_assert(0 != Singleton);
    Singleton = 0;
}

//------------------------------------------------------------------------------
/**
    Create a new memory animation object. Memory animations keep the
    entire animation data in memory, but are generally shared. Memory anims
    should be used when many objects are driven by the same animation.

    @param  rsrcName    the resource name for resource sharing
*/
nAnimation*
nAnimationServer::NewMemoryAnimation(const nString& rsrcName)
{
    n_assert(rsrcName.IsValid());
    return (nAnimation*)nResourceServer::Instance()->NewResource("nmemoryanimation", rsrcName, nResource::Animation);
}

//------------------------------------------------------------------------------
/**
    Create a new streaming animation. Streaming animations stream their
    data from disk, which has the advantage that animations can be very
    long (hours if you want), but each streaming animation needs its
    own small streaming buffer, which may add up when many objects are driven
    by streamed animations, also disk bandwidth and seek latency may become
    a limiting factor.

    Streaming animations generally cannot be shared, thus this method
    has no resource name parameter.
*/
nAnimation*
nAnimationServer::NewStreamingAnimation()
{
    return (nAnimation*)nResourceServer::Instance()->NewResource("nstreaminganimation", 0, nResource::Animation);
}
