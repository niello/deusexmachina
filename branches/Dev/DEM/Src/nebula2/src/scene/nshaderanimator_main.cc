//------------------------------------------------------------------------------
//  nshaderanimator_main.cc
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "scene/nshaderanimator.h"
#include <kernel/nkernelserver.h>

nNebulaClass(nShaderAnimator, "nanimator");

//------------------------------------------------------------------------------
/**
*/
nShaderAnimator::nShaderAnimator() :
    param(nShaderState::InvalidParameter)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nShaderAnimator::~nShaderAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Returns shader type.
*/
nAnimator::Type
nShaderAnimator::GetAnimatorType() const
{
    return Shader;
}

//------------------------------------------------------------------------------
/**
    Set the name of the shader parameter which should be animated by
    this object.
*/
void
nShaderAnimator::SetParamName(const char* paramName)
{
    n_assert(paramName);
    this->param = nShaderState::StringToParam(paramName);
}

//------------------------------------------------------------------------------
/**
    Get the name of the shader parameter which is animated by this object.
*/
const char*
nShaderAnimator::GetParamName() const
{
    if (nShaderState::InvalidParameter == this->param)
    {
        return 0;
    }
    return nShaderState::ParamToString(this->param);
}
