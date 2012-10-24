//------------------------------------------------------------------------------
//  nstatenode_main.cc
//  (C) 2005 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nskystate.h"
#include "gfx2/ngfxserver2.h"

nNebulaClass(nSkyState, "nabstractshadernode");

//------------------------------------------------------------------------------
/**
*/
nSkyState::nSkyState()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nSkyState::~nSkyState()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Attach to the scene server.
*/
void
nSkyState::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
{
	// Animate here
    nTransformNode::Attach(sceneServer, renderContext);
}

//------------------------------------------------------------------------------
/**
    nStateNodes doesn't provide transform
*/
bool
nSkyState::HasTransform() const
{
    return false;
}
