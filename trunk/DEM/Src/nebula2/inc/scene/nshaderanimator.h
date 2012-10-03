#ifndef N_SHADERANIMATOR_H
#define N_SHADERANIMATOR_H
//------------------------------------------------------------------------------
/**
    Base class for shader attribute animators.

    (C) 2005 Radon Labs GmbH
*/
#include "scene/nanimator.h"

//------------------------------------------------------------------------------
class nShaderAnimator : public nAnimator
{
public:
    /// constructor
    nShaderAnimator();
    /// destructor
    virtual ~nShaderAnimator();
    /// save object to persistent stream
    //virtual bool SaveCmds(nPersistServer* ps);
    /// return the type of this animator object (SHADER)
    virtual Type GetAnimatorType() const;
    /// set the name of the shader parameter to manipulate
    void SetParamName(const char* n);
    /// get the name of the attribute parameter to manipulate
    const char* GetParamName() const;

protected:
    nShaderState::Param param;
};
//------------------------------------------------------------------------------
#endif




