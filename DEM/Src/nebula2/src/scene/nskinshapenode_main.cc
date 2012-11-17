#include "scene/nskinshapenode.h"
#include "scene/nskinanimator.h"
#include <Data/BinaryReader.h>

nNebulaClass(nSkinShapeNode, "nshapenode");

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

				nArray<int>& Palette = fragmentArray[i].JointPalette;

				short PaletteSize;
				if (!DataReader.Read(PaletteSize)) FAIL;

				BeginJointPalette(i, PaletteSize);
				for (short j = 0; j < PaletteSize; ++j)
					Palette[j] = DataReader.Read<int>(); //!!!memcpy is more effective!
				EndJointPalette(i);
			}
			EndFragments();

			OK;
		}
		default: return nShapeNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

bool nSkinShapeNode::RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer && renderContext);

	//!!!???check once?
	n_assert(refMesh.isvalid() && refMesh->HasAllVertexComponents(nMesh2::Weights | nMesh2::JIndices));

    // call my skin animator (updates the char skeleton pointer)
    nKernelServer::Instance()->PushCwd(this);
	nSkinAnimator* pSkinAnimator = (nSkinAnimator*)nKernelServer::Instance()->Lookup(SkinAnimatorName.Get());
    if (pSkinAnimator) pSkinAnimator->Animate(this, renderContext);
    nKernelServer::Instance()->PopCwd();

    // render the skin in several passes (one per skin fragment)
    n_assert(extCharSkeleton);
    nGfxServer2::Instance()->SetMesh(refMesh, refMesh);
    for (int fragIndex = 0; fragIndex < GetNumFragments(); fragIndex++)
    {
        Fragment& fragment = this->fragmentArray[fragIndex];

		static const int maxJointPaletteSize = 72;
		static matrix44 jointArray[maxJointPaletteSize];

		// extract the current joint palette from the skeleton in the
		// right format for the skinning shader
		int paletteSize = fragment.JointPalette.Size();
		n_assert(paletteSize <= maxJointPaletteSize);
		for (int paletteIndex = 0; paletteIndex < paletteSize; paletteIndex++)
		{
			const nCharJoint& joint = extCharSkeleton->GetJointAt(fragment.JointPalette[paletteIndex]);
			jointArray[paletteIndex] = joint.GetSkinMatrix44();
		}

		// transfer the joint palette to the current shader
		nShader2* shd = nGfxServer2::Instance()->GetShader();
		n_assert(shd);
		if (shd->IsParameterUsed(nShaderState::JointPalette))
			shd->SetMatrixArray(nShaderState::JointPalette, jointArray, paletteSize);

		// set current vertex and index range and draw mesh
		const nMeshGroup& meshGroup = refMesh->Group(fragment.MeshGroupIdx);
		nGfxServer2::Instance()->SetVertexRange(meshGroup.FirstVertex, meshGroup.NumVertices);
		nGfxServer2::Instance()->SetIndexRange(meshGroup.FirstIndex, meshGroup.NumIndices);
		nGfxServer2::Instance()->DrawIndexedNS(nGfxServer2::TriangleList);
    }

    return true;
}
//---------------------------------------------------------------------

// Render joints
void nSkinShapeNode::RenderDebug(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix)
{
    n_assert(sceneServer && renderContext);

    // call my skin animator (updates the char skeleton pointer)
    nKernelServer::Instance()->PushCwd(this);
	nSkinAnimator* pSkinAnimator = (nSkinAnimator*)nKernelServer::Instance()->Lookup(SkinAnimatorName.Get());
    if (pSkinAnimator) pSkinAnimator->Animate(this, renderContext);
    nKernelServer::Instance()->PopCwd();

    nArray<vector3> lines;
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    gfxServer->BeginShapes();

    for (int i = 0; i < extCharSkeleton->GetNumJoints(); i++)
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
            float blue = (1.f / extCharSkeleton->GetNumJoints() * (i+1));
            float red = (1.f - 1.f / extCharSkeleton->GetNumJoints() * (i+1));
            vector4 color(red, 1.f, blue, 1.f);
            gfxServer->DrawShapePrimitives(nGfxServer2::LineList, lines.Size(), &(lines[0]), 3, modelMatrix, color);
        }
    }

    gfxServer->EndShapes();

    // call parrent
    nShapeNode::RenderDebug(sceneServer, renderContext, modelMatrix);
}
//---------------------------------------------------------------------
