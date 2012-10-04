//------------------------------------------------------------------------------
//  nmultilayerednode_main.cc
//  (C) 2005 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nmultilayerednode.h"
#include "variable/nvariableserver.h"
#include "scene/nrendercontext.h"
#include "scene/nsceneserver.h"
#include "scene/nanimator.h"

nNebulaClass(nMultiLayeredNode, "nshapenode");

//------------------------------------------------------------------------------
/**
*/
nMultiLayeredNode::nMultiLayeredNode()
{
    texCount = 0;
}

//------------------------------------------------------------------------------
/**
*/
nMultiLayeredNode::~nMultiLayeredNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
nMultiLayeredNode::RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    // build uv-value-matrix for dx9 shader
    matrix44 uvStretchMatrix;
    for (int i = 0; i < this->texCount; i++)
        uvStretchMatrix.m[i/4][i%4] = this->uvStretch[i];

    nShader2* shd = nGfxServer2::Instance()->GetShader();
    if (shd->IsParameterUsed(nShaderState::MLPUVStretch))
    {
        shd->SetMatrix(nShaderState::MLPUVStretch, uvStretchMatrix);
    }

    // set uv matrices for dx7 shader
    for (int i = 0; i < 5; i++)
    {
        float stretch = this->dx7uvStretch[i];
        matrix44 uvStretch;
        uvStretch.ident();
        uvStretch.m[0][0] = stretch;
        uvStretch.m[1][1] = stretch;
        uvStretch.m[2][2] = stretch;
        uvStretch.m[3][3] = stretch;
        switch (i)
        {
            case 0: if (shd->IsParameterUsed(nShaderState::UVStretch0)) shd->SetMatrix(nShaderState::UVStretch0, uvStretch); break;
            case 1: if (shd->IsParameterUsed(nShaderState::UVStretch1)) shd->SetMatrix(nShaderState::UVStretch1, uvStretch); break;
            case 2: if (shd->IsParameterUsed(nShaderState::UVStretch2)) shd->SetMatrix(nShaderState::UVStretch2, uvStretch); break;
            case 3: if (shd->IsParameterUsed(nShaderState::UVStretch3)) shd->SetMatrix(nShaderState::UVStretch3, uvStretch); break;
            case 4: if (shd->IsParameterUsed(nShaderState::UVStretch4)) shd->SetMatrix(nShaderState::UVStretch4, uvStretch); break;
        }
    }
    return nShapeNode::RenderGeometry(sceneServer, renderContext);
}
