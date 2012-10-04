//------------------------------------------------------------------------------
//  nvectoranimator_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nvectoranimator.h"
#include "scene/nabstractshadernode.h"
#include "scene/nrendercontext.h"

nNebulaClass(nVectorAnimator, "nshaderanimator");

//------------------------------------------------------------------------------
/**
*/
nVectorAnimator::nVectorAnimator() :
    keyArray(0, 4)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nVectorAnimator::~nVectorAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Add a key to the animation key array.

    @param time time in seconds.
    @param key key values.
*/
void
nVectorAnimator::AddKey(float time, const vector4& key)
{
    nAnimKey<vector4> newKey(time, key);
    this->keyArray.Append(newKey);
}

//------------------------------------------------------------------------------
/**
    Return the number of keys in the animation key array.
*/
int
nVectorAnimator::GetNumKeys() const
{
    return this->keyArray.Size();
}

//------------------------------------------------------------------------------
/**
    Return information for a key index.
*/
void
nVectorAnimator::GetKeyAt(int index, float& time, vector4& key) const
{
    const nAnimKey<vector4>& animKey = this->keyArray[index];
    time = animKey.GetTime();
    key  = animKey.GetValue();
}

//------------------------------------------------------------------------------
/**
*/
void
nVectorAnimator::Animate(nSceneNode* sceneNode, nRenderContext* renderContext)
{
    n_assert(sceneNode);
    n_assert(renderContext);
    n_assert(nVariable::InvalidHandle != this->channelVarHandle);
    n_assert(sceneNode->IsA("nabstractshadernode"));

    nAbstractShaderNode* targetNode = (nAbstractShaderNode*) sceneNode;

    // get the sample time from the render context
    nVariable* var = renderContext->GetVariable(this->channelVarHandle);
    n_assert(var);
    float curTime = var->GetFloat();

    // get sampled key
    static nAnimKey<vector4> key;
    if (this->keyArray.Sample(curTime, this->loopType, key))
    {
        targetNode->SetVector(this->param, key.GetValue());
    }
}

