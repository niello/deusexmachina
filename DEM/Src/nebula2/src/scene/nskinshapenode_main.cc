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

			n_assert(Count > 0);
			Fragments.SetFixedSize(Count);
			for (short i = 0; i < Count; ++i)
			{
				//!!!REDUNDANCY! { [ { } ] } problem.
				// This is { }'s element count, which is always 2 (MeshGroupIndex, JointPalette)
				n_assert(DataReader.Read<short>() == 2);

				DataReader.Read<int>(Fragments[i].MeshGroupIdx);

				nArray<int>& Palette = Fragments[i].JointPalette;

				short PaletteSize;
				if (!DataReader.Read(PaletteSize)) FAIL;

				Fragments[i].JointPalette.SetFixedSize(PaletteSize);
				DataReader.GetStream().Read(Palette.Begin(), PaletteSize * sizeof(int));
			}

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

	nShader2* pShd = nGfxServer2::Instance()->GetShader();
	n_assert(pShd);
	bool ShaderUsesPalette = pShd->IsParameterUsed(nShaderState::JointPalette);

	n_assert(pSkeleton);
    nGfxServer2::Instance()->SetMesh(refMesh, refMesh);
    for (int FragIdx = 0; FragIdx < Fragments.Size(); ++FragIdx)
    {
        Fragment& Frag = Fragments[FragIdx];

		if (ShaderUsesPalette)
		{
			static const int MAX_BONES_PER_PASS = 72;
			static matrix44 FragJoints[MAX_BONES_PER_PASS];

			// extract the current joint palette from the skeleton in the right format for the skinning shader
			n_assert(Frag.JointPalette.Size() <= MAX_BONES_PER_PASS);
			for (int j = 0; j < Frag.JointPalette.Size(); ++j)
				FragJoints[j] = pSkeleton->GetJointAt(Frag.JointPalette[j]).GetSkinMatrix44();

			pShd->SetMatrixArray(nShaderState::JointPalette, FragJoints, Frag.JointPalette.Size());
		}

		// set current vertex and index range and draw mesh
		const nMeshGroup& meshGroup = refMesh->Group(Frag.MeshGroupIdx);
		nGfxServer2::Instance()->SetVertexRange(meshGroup.FirstVertex, meshGroup.NumVertices);
		nGfxServer2::Instance()->SetIndexRange(meshGroup.FirstIndex, meshGroup.NumIndices);
		nGfxServer2::Instance()->DrawIndexedNS(nGfxServer2::TriangleList);
    }

    return true;
}
//---------------------------------------------------------------------
