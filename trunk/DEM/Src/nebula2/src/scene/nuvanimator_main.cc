//------------------------------------------------------------------------------
//  nuvanimator_main.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nuvanimator.h"
#include "scene/nrendercontext.h"
#include "scene/nabstractshadernode.h"

nNebulaClass(nUvAnimator, "nanimator");

//------------------------------------------------------------------------------
/**
*/
nUvAnimator::nUvAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nUvAnimator::~nUvAnimator()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nAnimator::Type
nUvAnimator::GetAnimatorType() const
{
    return Shader;
}

//------------------------------------------------------------------------------
/**
    This does the actual work of manipulate the target object.

    @param  sceneNode       object to manipulate (must be of class nTransformNode)
    @param  renderContext   current render context

    -01-Nov-06  kims  Updated to be enable to animate texture uvs.
                      Thank ZHANG Zikai for the patch.
*/
void
nUvAnimator::Animate(nSceneNode* sceneNode, nRenderContext* renderContext)
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

    static nAnimKey<vector2> keypos;
    static nAnimKey<vector3> keyeuler;
    static nAnimKey<vector2> keyscale;
    bool bkeypos, bkeyeuler, bkeyscale;

    // sample keys
    bkeypos = this->posArray->Sample(curTime, this->loopType, keypos);
    bkeyeuler = this->eulerArray->Sample(curTime, this->loopType, keyeuler);
    bkeyscale = this->scaleArray->Sample(curTime, this->loopType, keyscale);

    int texLayer;
    for (texLayer = 0; texLayer < nGfxServer2::MaxTextureStages; texLayer++)
    {
        // manipulate target object
        if (bkeypos)
        {
            targetNode->SetUvPos(texLayer, keypos.GetValue());
        }
        if (bkeyeuler)
        {
            targetNode->SetUvEuler(texLayer, keyeuler.GetValue());
        }
        if (bkeyscale)
        {
            targetNode->SetUvScale(texLayer, keyscale.GetValue());
        }
    }

    // apply the texture transforms
    nGfxServer2 *gfxServer = nGfxServer2::Instance();
    if (!gfxServer->GetHint(nGfxServer2::MvpOnly))
    {
        // set texture transforms
        n_assert(nGfxServer2::MaxTextureStages >= 4);

        // inorder to match the 3dsmax uv-animation
        static matrix33 m3;
        static matrix44 m;
        m3.ident();
        if (bkeyscale || bkeyeuler)
        {
            m3.translate(vector2(-.5f, -.5f));
        }
        if (bkeypos)
        {
            m3.translate(vector2(-keypos.GetValue().x, keypos.GetValue().y));
        }
        if (bkeyscale)
        {
            m3.scale(vector3(keyscale.GetValue().x, keyscale.GetValue().y, 1));
        }
        if (bkeyeuler)
        {
            m3.rotate_x(keyeuler.GetValue().x);
            m3.rotate_y(keyeuler.GetValue().y);
            m3.rotate_z(keyeuler.GetValue().z);
        }
        if (bkeyscale || bkeyeuler)
        {
            m3.translate(vector2(.5f, .5f));
        }
        m.set(m3.M11, m3.M12, m3.M13, 0,
            m3.M21, m3.M22, m3.M23, 0,
            m3.M31, m3.M32, m3.M33, 0,
            0, 0, 0, 1);

        gfxServer->SetTransform(nGfxServer2::Texture0, m);
        gfxServer->SetTransform(nGfxServer2::Texture1, m);
    }
}

