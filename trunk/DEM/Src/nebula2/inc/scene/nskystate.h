#ifndef N_SKYSTATE_H
#define N_SKYSTATE_H
//------------------------------------------------------------------------------
/**
    @class nSkyState
    @ingroup Scene

    Provides data for animated or state switching nodes

    (C) 2005 RadonLabs GmbH
*/
#include "scene/nabstractshadernode.h"

//------------------------------------------------------------------------------
class nSkyState : public nAbstractShaderNode
{
public:
    /// constructor
    nSkyState();
    /// destructor
    virtual ~nSkyState();
    /// nStateNodes doesn't provide transform
    virtual bool HasTransform () const ;
    /// Attach to sceneserver
    void Attach(nSceneServer* sceneServer, nRenderContext* renderContext);

private:
    nString shaderName;
};
//-----------------------------------------------------------------------------
#endif

