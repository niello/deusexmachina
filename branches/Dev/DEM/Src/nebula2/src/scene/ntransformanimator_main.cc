//------------------------------------------------------------------------------
//  ntransformanimator_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/ntransformanimator.h"
#include "scene/nrendercontext.h"
#include "scene/ntransformnode.h"
#include <kernel/nkernelserver.h>

nNebulaClass(nTransformAnimator, "nanimator");

//------------------------------------------------------------------------------
/**
*/
nTransformAnimator::nTransformAnimator() :
    posArray(0, 4),
    eulerArray(0, 4),
    scaleArray(0, 4),
    quatArray(0, 4)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nTransformAnimator::~nTransformAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nAnimator::Type
nTransformAnimator::GetAnimatorType() const
{
    return Transform;
}

//------------------------------------------------------------------------------
/**
    This does the actual work of manipulate the target object.

    @param  sceneNode       object to manipulate (must be of class nTransformNode)
    @param  renderContext   current render context
*/
void
nTransformAnimator::Animate(nSceneNode* sceneNode, nRenderContext* renderContext)
{
    n_assert(sceneNode);
    n_assert(renderContext);
    n_assert(nVariable::InvalidHandle != this->channelVarHandle);

    // FIXME: dirty cast, make sure that it is a nTransformNode
    nTransformNode* targetNode = (nTransformNode*) sceneNode;

    // get the sample time from the render context
    nVariable* var = renderContext->GetVariable(this->channelVarHandle);
    float curTime = 0.0f;
    if (0 == var)
    {
        n_printf("Warning: nTransformAnimator::Animate() ChannelVariable '%s' not found!\n", this->GetChannel());
    }
    else
    {
        curTime = var->GetFloat();
    }

    // sample key arrays and manipulate target object
    static nAnimKey<vector3> key;
    static nAnimKey<quaternion> quatkey;
    if (this->posArray.Sample(curTime, this->loopType, key))
    {
        targetNode->SetPosition(key.GetValue());
    }
    if (this->quatArray.Sample(curTime, this->loopType, quatkey))
    {
        targetNode->SetQuat(quatkey.GetValue());
    }
    if (this->eulerArray.Sample(curTime, this->loopType, key))
    {
        targetNode->SetEuler(key.GetValue());
    }
    if (this->scaleArray.Sample(curTime, this->loopType, key))
    {
        targetNode->SetScale(key.GetValue());
    }
}
