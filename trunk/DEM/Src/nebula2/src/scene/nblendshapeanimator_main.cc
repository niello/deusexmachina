//------------------------------------------------------------------------------
//  nblendshapeanimator_main.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nblendshapeanimator.h"
#include "scene/nrendercontext.h"
#include "scene/nblendshapenode.h"

nNebulaClass(nBlendShapeAnimator, "nanimator");

//------------------------------------------------------------------------------
/**
*/
nBlendShapeAnimator::nBlendShapeAnimator() :
    animationGroup(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nBlendShapeAnimator::~nBlendShapeAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nAnimator::Type
nBlendShapeAnimator::GetAnimatorType() const
{
    return nAnimator::BlendShape;
}

//------------------------------------------------------------------------------
/**
    Unload animation resource if valid.
*/
void
nBlendShapeAnimator::UnloadAnimation()
{
    if (this->refAnimation.isvalid())
    {
        this->refAnimation->Release();
        this->refAnimation.invalidate();
    }
}

//------------------------------------------------------------------------------
/**
    Load new animation, release old one if valid.
*/
bool
nBlendShapeAnimator::LoadAnimation()
{
    if ((!this->refAnimation.isvalid()) && (!this->animationName.IsEmpty()))
    {
        nAnimation* animation = nAnimationServer::Instance()->NewMemoryAnimation(this->animationName);
        n_assert(animation);
        if (!animation->IsLoaded())
        {
            animation->SetFilename(this->animationName);
            if (!animation->Load())
            {
                n_printf("nBlendShapeAnimator: Error loading animation '%s'\n", this->animationName.Get());
                animation->Release();
                return false;
            }
        }
        this->refAnimation = animation;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Load the resources needed by this object.
*/
bool
nBlendShapeAnimator::LoadResources()
{
    if (this->LoadAnimation())
    {
        this->resourcesValid = true;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Unload the resources if refcount has reached zero.
*/
void
nBlendShapeAnimator::UnloadResources()
{
    this->UnloadAnimation();
    this->resourcesValid = false;
}

//------------------------------------------------------------------------------
/**
    This does the actual work of manipulate the target object.

    @param  sceneNode       object to manipulate (must be of class nBlendShapeNode)
    @param  renderContext   current render context
*/
void
nBlendShapeAnimator::Animate(nSceneNode* sceneNode, nRenderContext* renderContext)
{
    n_assert(sceneNode);
    n_assert(renderContext);
    n_assert(nVariable::InvalidHandle != this->channelVarHandle);
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }
    n_assert(this->refAnimation->IsLoaded());

    // FIXME: dirty cast, make sure that it is a nBlendShapeNode
    nBlendShapeNode* targetNode = (nBlendShapeNode*) sceneNode;

    // get the sample time from the render context
    nVariable* var = renderContext->GetVariable(this->channelVarHandle);
    n_assert(var);
    float curTime = var->GetFloat();

    // sample curves and manipulate target object
    vector4 keyArray[nBlendShapeNode::MaxShapes];
    int numCurves = this->refAnimation->GetGroupAt(0).GetNumCurves();
    this->refAnimation->SampleCurves(curTime, this->animationGroup, 0, numCurves, keyArray);
    int curveIndex;
    for (curveIndex = 0; curveIndex < numCurves; curveIndex++)
    {
        targetNode->SetWeightAt(curveIndex, keyArray[curveIndex].x);
    }
}
