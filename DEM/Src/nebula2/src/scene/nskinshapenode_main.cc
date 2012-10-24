//------------------------------------------------------------------------------
//  nskinshapenode_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nskinshapenode.h"
#include "scene/nskinanimator.h"
#include <Data/BinaryReader.h>

nNebulaClass(nSkinShapeNode, "nshapenode");

//------------------------------------------------------------------------------
/**
*/
nSkinShapeNode::nSkinShapeNode() :
    extCharSkeleton(0),
    isChar3AndBoundToVariation(false)
{
    // empty
}

bool nSkinShapeNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'NAKS': // SKAN
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetSkinAnimator(Value);
			OK;
		}
		case 'MGRF': // FRGM
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;

			BeginFragments(Count);
			for (short i = 0; i < Count; ++i)
			{
				SetFragGroupIndex(i, DataReader.Read<int>());

				nCharJointPalette& Palette = fragmentArray[i].GetJointPalette();

				short PaletteSize;
				if (!DataReader.Read(PaletteSize)) FAIL;

				BeginJointPalette(i, PaletteSize);
				for (short j = 0; j < PaletteSize; ++j)
					Palette.SetJointIndex(j, DataReader.Read<int>()); //!!!memcpy is more effective!
				EndJointPalette(i);
			}
			EndFragments();

			OK;
		}
		default: return nShapeNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Returns the character skeleton object
*/
const nCharSkeleton*
nSkinShapeNode::GetCharSkeleton() const
{
    return this->extCharSkeleton;
}

//------------------------------------------------------------------------------
/**
    This method must return the mesh usage flag combination required by
    this shape node class. Subclasses should override this method
    based on their requirements.

    @return     a combination on nMesh2::Usage flags
*/
int
nSkinShapeNode::GetMeshUsage() const
{
    return nMesh2::WriteOnce | nMesh2::NeedsVertexShader;
}

//------------------------------------------------------------------------------
/**
*/
bool
nSkinShapeNode::ApplyGeometry(nSceneServer* /*sceneServer*/)
{
    if (!this->charSkinRenderer.IsInitialized())
    {
        // never use cpu skinning
        this->charSkinRenderer.Initialize(this->refMesh);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    This overrides the standard nShapeNode geometry rendering. Instead
    of setting the mesh directly in the gfx server, the embedded
    nCharSkinRenderer is asked to do the rendering for us.

    - 15-Jan-04     floh    AreResourcesValid()/LoadResource() moved to scene server
*/
bool
nSkinShapeNode::RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    // call my skin animator (updates the char skeleton pointer)
    nKernelServer::Instance()->PushCwd(this);
	nSkinAnimator* pSkinAnimator = (nSkinAnimator*)nKernelServer::Instance()->Lookup(SkinAnimatorName.Get());
    if (pSkinAnimator) pSkinAnimator->Animate(this, renderContext);
    nKernelServer::Instance()->PopCwd();

    // render the skin in several passes (one per skin fragment)
    this->charSkinRenderer.Begin(this->extCharSkeleton);
    int numFragments = this->GetNumFragments();
    int fragIndex;
    for (fragIndex = 0; fragIndex < numFragments; fragIndex++)
    {
        Fragment& fragment = this->fragmentArray[fragIndex];
        charSkinRenderer.Render(fragment.GetMeshGroupIndex(), fragment.GetJointPalette());
    }
    this->charSkinRenderer.End();

    return true;
}

//------------------------------------------------------------------------------
/**
    Render debugging information for this mesh (the joints)
*/
void
nSkinShapeNode::RenderDebug(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    // call my skin animator (updates the char skeleton pointer)
    nKernelServer::Instance()->PushCwd(this);
	nSkinAnimator* pSkinAnimator = (nSkinAnimator*)nKernelServer::Instance()->Lookup(SkinAnimatorName.Get());
    if (pSkinAnimator) pSkinAnimator->Animate(this, renderContext);
    nKernelServer::Instance()->PopCwd();

    // render the joints of the skeleton
    int numJoints = this->extCharSkeleton->GetNumJoints();
    int i;

    nArray<vector3> lines;
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    gfxServer->BeginShapes();

    for (i = 0; i < numJoints; i++)
    {
        // add a joint
        lines.Clear();
        const nCharJoint& joint = this->extCharSkeleton->GetJointAt(i);
        if (joint.GetParentJointIndex() != -1)
        {
            nCharJoint* parentJoint = joint.GetParentJoint();
            n_assert(parentJoint);

            // add start point
            lines.Append(parentJoint->GetMatrix() * vector3());

            // add end point
            lines.Append(joint.GetMatrix() * vector3());

            // draw
            float blue = (1.f / numJoints * (i+1));
            float red = (1.f - 1.f / numJoints * (i+1));
            vector4 color(red, 1.f, blue, 1.f);
            gfxServer->DrawShapePrimitives(nGfxServer2::LineList, lines.Size(), &(lines[0]), 3, modelMatrix, color);
        }
    }

    gfxServer->EndShapes();

    // call parrent
    nShapeNode::RenderDebug(sceneServer, renderContext, modelMatrix);
}

//------------------------------------------------------------------------------
/**
    Set relative path to the skin animator object.
*/
void
nSkinShapeNode::SetSkinAnimator(const char* path)
{
    n_assert(path);
    SkinAnimatorName = path;
}

//------------------------------------------------------------------------------
/**
    Get relative path to the skin animator object
*/
const char*
nSkinShapeNode::GetSkinAnimator() const
{
    return SkinAnimatorName.Get();
}

//------------------------------------------------------------------------------
/**
    Update the pointer to an uptodate nCharSkeleton object. This pointer
    is provided by the nSkinAnimator object and is routed to the
    nCharSkinRenderer so that the mesh can be properly deformed.
*/
void
nSkinShapeNode::SetCharSkeleton(const nCharSkeleton* charSkeleton)
{
    n_assert(charSkeleton);
    this->extCharSkeleton = charSkeleton;
}

//------------------------------------------------------------------------------
/**
    Begin defining mesh fragment. A skin mesh may be divided into several
    fragments to account for gfx hardware which an only render a skinned
    mesh with a limited number of influence objects (joints).
*/
void
nSkinShapeNode::BeginFragments(int num)
{
    n_assert(num > 0);
    this->fragmentArray.SetFixedSize(num);
}

//------------------------------------------------------------------------------
/**
    Set the mesh group index for a skin fragment.
*/
void
nSkinShapeNode::SetFragGroupIndex(int fragIndex, int meshGroupIndex)
{
    this->fragmentArray[fragIndex].SetMeshGroupIndex(meshGroupIndex);
}

//------------------------------------------------------------------------------
/**
    Get the mesh group index for a skin fragment.
*/
int
nSkinShapeNode::GetFragGroupIndex(int fragIndex) const
{
    return this->fragmentArray[fragIndex].GetMeshGroupIndex();
}

//------------------------------------------------------------------------------
/**
    Begin defining the joint palette of a skin fragment.
*/
void
nSkinShapeNode::BeginJointPalette(int fragIndex, int numJoints)
{
    this->fragmentArray[fragIndex].GetJointPalette().BeginJoints(numJoints);
}

//------------------------------------------------------------------------------
/**
    Add up to 8 joints to a fragments joint palette starting at a given
    palette index.
*/
void
nSkinShapeNode::SetJointIndices(int fragIndex, int paletteIndex, int ji0, int ji1, int ji2, int ji3, int ji4, int ji5, int ji6, int ji7)
{
    nCharJointPalette& pal = this->fragmentArray[fragIndex].GetJointPalette();
    int numJoints = pal.GetNumJoints();
    if (paletteIndex < numJoints) pal.SetJointIndex(paletteIndex++, ji0);
    if (paletteIndex < numJoints) pal.SetJointIndex(paletteIndex++, ji1);
    if (paletteIndex < numJoints) pal.SetJointIndex(paletteIndex++, ji2);
    if (paletteIndex < numJoints) pal.SetJointIndex(paletteIndex++, ji3);
    if (paletteIndex < numJoints) pal.SetJointIndex(paletteIndex++, ji4);
    if (paletteIndex < numJoints) pal.SetJointIndex(paletteIndex++, ji5);
    if (paletteIndex < numJoints) pal.SetJointIndex(paletteIndex++, ji6);
    if (paletteIndex < numJoints) pal.SetJointIndex(paletteIndex++, ji7);
}

//------------------------------------------------------------------------------
/**
*/
void
nSkinShapeNode::SetJointIndex(int fragIndex, int paletteIndex, int jointIndex)
{
    nCharJointPalette& pal = this->fragmentArray[fragIndex].GetJointPalette();
    pal.SetJointIndex(paletteIndex, jointIndex);
}

//------------------------------------------------------------------------------
/**
    Finish defining the joint palette of a skin fragment.
*/
void
nSkinShapeNode::EndJointPalette(int fragIndex)
{
    this->fragmentArray[fragIndex].GetJointPalette().EndJoints();
}

//------------------------------------------------------------------------------
/**
    Finish defining fragments.
*/
void
nSkinShapeNode::EndFragments()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Get number of fragments.
*/
int
nSkinShapeNode::GetNumFragments() const
{
    return this->fragmentArray.Size();
}

//------------------------------------------------------------------------------
/**
    Get joint palette size of a skin fragment.
*/
int
nSkinShapeNode::GetJointPaletteSize(int fragIndex) const
{
    return this->fragmentArray[fragIndex].GetJointPalette().GetNumJoints();
}

//------------------------------------------------------------------------------
/**
    Get a joint index from a fragment's joint index.
*/
int
nSkinShapeNode::GetJointIndex(int fragIndex, int paletteIndex) const
{
    return this->fragmentArray[fragIndex].GetJointPalette().GetJointIndexAt(paletteIndex);
}

