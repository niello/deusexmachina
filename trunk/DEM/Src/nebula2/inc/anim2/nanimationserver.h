#ifndef N_ANIMATIONSERVER_H
#define N_ANIMATIONSERVER_H
//------------------------------------------------------------------------------
/**
    @class nAnimationServer
    @ingroup Anim2

    @brief The animation server offers central services of the animation
    subsystem (at the moment it is merely a factory for nAnimation objects).

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/nroot.h"

class nResourceServer;
class nAnimation;

//------------------------------------------------------------------------------
class nAnimationServer: public nReferenced
{
public:
    /// constructor
    nAnimationServer();
    /// destructor
    virtual ~nAnimationServer();
    /// return instance pointer
    static nAnimationServer* Instance();
    /// create an in-memory animation object (always shared)
    virtual nAnimation* NewMemoryAnimation(const nString& rsrcName);
    /// create a new streaming animation object (never shared)
    virtual nAnimation* NewStreamingAnimation();

private:
    static nAnimationServer* Singleton;
};

//------------------------------------------------------------------------------
/**
*/
inline
nAnimationServer*
nAnimationServer::Instance()
{
    n_assert(Singleton);
    return Singleton;
}

//------------------------------------------------------------------------------
#endif
