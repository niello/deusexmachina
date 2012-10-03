//------------------------------------------------------------------------------
//  nanimeventhandler.cc
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "anim2/nanimeventhandler.h"

//------------------------------------------------------------------------------
/**
*/
nAnimEventHandler::nAnimEventHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nAnimEventHandler::~nAnimEventHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
nAnimEventHandler::HandleEvent(const nAnimEventTrack& track, int eventIndex)
{
    n_printf("nAnimEventHandler::HandleEvent(%s, %f)\n", track.GetName().Get(), track.GetEvent(eventIndex).GetTime());
}


