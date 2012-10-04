//------------------------------------------------------------------------------
//  nfloatanimator_main.cc
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "scene/nfloatanimator.h"
#include "scene/nabstractshadernode.h"
#include "scene/nrendercontext.h"

nNebulaClass(nFloatAnimator, "nshaderanimator");

//------------------------------------------------------------------------------
/**
*/
nFloatAnimator::nFloatAnimator() :
    keyArray(0, 4)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nFloatAnimator::~nFloatAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Add a key to the animation key array.
*/
void
nFloatAnimator::AddKey(float time, float key)
{
    nAnimKey<float> newKey(time, key);
    this->keyArray.Append(newKey);
}

//------------------------------------------------------------------------------
/**
    Return the number of keys in the animation key array.
*/
int
nFloatAnimator::GetNumKeys() const
{
    return this->keyArray.Size();
}

//------------------------------------------------------------------------------
/**
    Return information for a key index.
*/
void
nFloatAnimator::GetKeyAt(int index, float& time, float& key) const
{
    const nAnimKey<float>& animKey = this->keyArray[index];
    time = animKey.GetTime();
    key  = animKey.GetValue();
}

//------------------------------------------------------------------------------
/**
*/
void
nFloatAnimator::Animate(nSceneNode* sceneNode, nRenderContext* renderContext)
{
    n_assert(sceneNode);
    n_assert(renderContext);
    n_assert(nVariable::InvalidHandle != this->channelVarHandle);

    // FIXME: dirty cast, make sure that it is a nAbstractShaderNode!
    nAbstractShaderNode* targetNode = (nAbstractShaderNode*)sceneNode;

    // get the sample time from the render context
    nVariable* var = renderContext->GetVariable(this->channelVarHandle);
    n_assert(var);
    float curTime = var->GetFloat();

    // get sampled key
    static nAnimKey<float> key;
    if (this->keyArray.Sample(curTime, this->loopType, key))
    {
        targetNode->SetFloat(this->param, key.GetValue());
    }
}

















