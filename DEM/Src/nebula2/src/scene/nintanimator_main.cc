//------------------------------------------------------------------------------
//  nintanimator_main.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nintanimator.h"
#include "scene/nabstractshadernode.h"
#include "scene/nrendercontext.h"

nNebulaClass(nIntAnimator, "nshaderanimator");

//------------------------------------------------------------------------------
/**
*/
nIntAnimator::nIntAnimator() :
    keyArray(0, 4)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nIntAnimator::~nIntAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Add a key to the animation key array.
*/
void
nIntAnimator::AddKey(float time, int key)
{
    nAnimKey<int> newKey(time, key);
    this->keyArray.Append(newKey);
}

//------------------------------------------------------------------------------
/**
    Return the number of keys in the animation key array.
*/
int
nIntAnimator::GetNumKeys() const
{
    return this->keyArray.Size();
}

//------------------------------------------------------------------------------
/**
    Return information for a key index.
*/
void
nIntAnimator::GetKeyAt(int index, float& time, int& key) const
{
    const nAnimKey<int>& animKey = this->keyArray[index];
    time = animKey.GetTime();
    key  = animKey.GetValue();
}

//------------------------------------------------------------------------------
/**
*/
void
nIntAnimator::Animate(nSceneNode* sceneNode, nRenderContext* renderContext)
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
    static nAnimKey<int> key;
    if (this->keyArray.Sample(curTime, this->loopType, key))
    {
        targetNode->SetInt(this->param, key.GetValue());
    }
}
