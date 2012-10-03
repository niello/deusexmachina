//------------------------------------------------------------------------------
//  ncharskinrenderer.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "character/ncharskinrenderer.h"
#include "gfx2/nshader2.h"

//------------------------------------------------------------------------------
/**
*/
nCharSkinRenderer::nCharSkinRenderer() :
    initialized(false),
    inBegin(false),
    charSkeleton(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nCharSkinRenderer::~nCharSkinRenderer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This initialized the character skin renderer.
*/
bool
nCharSkinRenderer::Initialize(nMesh2* srcMesh)
{
    n_assert(!this->initialized);
    n_assert(srcMesh);
    n_assert(srcMesh->HasAllVertexComponents(nMesh2::Weights | nMesh2::JIndices));

    this->initialized = true;
    this->refSrcMesh = srcMesh;
    this->charSkeleton = 0;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
nCharSkinRenderer::Begin(const nCharSkeleton* skel)
{
    n_assert(!this->inBegin);
    n_assert(skel);

    this->charSkeleton = skel;
    this->inBegin = true;
    nGfxServer2::Instance()->SetMesh(this->refSrcMesh, this->refSrcMesh);
}

//------------------------------------------------------------------------------
/**
*/
void
nCharSkinRenderer::End()
{
    n_assert(this->inBegin);
    this->inBegin = false;
}

//------------------------------------------------------------------------------
/**
*/
void
nCharSkinRenderer::Render(int meshGroupIndex, nCharJointPalette& jointPalette)
{
    n_assert(inBegin);
    this->RenderShaderSkinning(meshGroupIndex, jointPalette);
}

//------------------------------------------------------------------------------
/**
    Render the skinned character with vertex shader skinning.
*/
void
nCharSkinRenderer::RenderShaderSkinning(int meshGroupIndex, nCharJointPalette& jointPalette)
{
    static const int maxJointPaletteSize = 72;
    static matrix44 jointArray[maxJointPaletteSize];

    // extract the current joint palette from the skeleton in the
    // right format for the skinning shader
    int paletteSize = jointPalette.GetNumJoints();
    n_assert(paletteSize <= maxJointPaletteSize);
    int paletteIndex;
    for (paletteIndex = 0; paletteIndex < paletteSize; paletteIndex++)
    {
        const nCharJoint& joint = this->charSkeleton->GetJointAt(jointPalette.GetJointIndexAt(paletteIndex));
        jointArray[paletteIndex] = joint.GetSkinMatrix44();
    }

    // transfer the joint palette to the current shader
    nShader2* shd = nGfxServer2::Instance()->GetShader();
    n_assert(shd);
    if (shd->IsParameterUsed(nShaderState::JointPalette))
    {
        shd->SetMatrixArray(nShaderState::JointPalette, jointArray, paletteSize);
    }

    // set current vertex and index range and draw mesh
    const nMeshGroup& meshGroup = this->refSrcMesh->Group(meshGroupIndex);
    nGfxServer2::Instance()->SetVertexRange(meshGroup.FirstVertex, meshGroup.NumVertices);
    nGfxServer2::Instance()->SetIndexRange(meshGroup.FirstIndex, meshGroup.NumIndices);
    nGfxServer2::Instance()->DrawIndexedNS(nGfxServer2::TriangleList);
}
