#include "Bone.h"

#include <Scene/SceneNode.h>
#include <Scene/Model.h>
#include <Render/RenderServer.h>
#include <Data/BinaryReader.h>

namespace Scene
{
ImplementRTTI(Scene::CBone, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CBone);

bool CBone::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'TESP': // PSET
		{
			return DataReader.Read(BindPoseLocal.Translation);
		}
		case 'RESP': // PSER
		{
			return DataReader.Read(BindPoseLocal.Rotation);
		}
		case 'SESP': // PSES
		{
			return DataReader.Read(BindPoseLocal.Scale);
		}
		case 'XDIB': // BIDX
		{
			return DataReader.Read(Index);
		}
		case 'PTNB': // BNTP
		{
			char BoneType[64];
			if (!DataReader.ReadString(BoneType, 63)) FAIL;
			if (!n_stricmp(BoneType, "root")) Flags.Set(Bone_Root);
			else if (!n_stricmp(BoneType, "term")) Flags.Set(Bone_Term);
			else FAIL;
			OK;
		}
		default: return CSceneNodeAttr::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

bool CBone::OnAdd()
{
	// Constructed as identity, no need to set explicitly now
	//SkinMatrix.ident();

	// Since SkinMatrix is identity and node position doesn't change, it is not necessary:
	// To undo inv. bind pose in initial state
	pNode->SetLocalTransform(BindPoseLocal);

	BindPoseLocal.ToMatrix(BindPoseWorld);

	CBone* pParentBone = GetParentBone();
	if (pParentBone) BindPoseWorld.mult_simple(pParentBone->GetBindPoseMatrix());

	BindPoseWorld.invert_simple(InvBindPose);

	CBone* pRootBone = GetRootBone();
	CSceneNode* pModelNode = pRootBone->pNode->GetParent() ? pRootBone->pNode->GetParent() : pRootBone->pNode;

	static CStrID sidJointPalette("JointPalette");

	// Find all models in model node and setup matrix pointers in a JointPalette shader var
	for (DWORD i = 0; i < pModelNode->GetAttrCount(); ++i)
	{
		CSceneNodeAttr* pAttr = pModelNode->GetAttr(i);
		if (pAttr->IsA(CModel::RTTI) &&
			(((CModel*)pAttr)->FeatureFlags & RenderSrv->GetFeatureFlagSkinned()) &&
			((CModel*)pAttr)->Material.isvalid())
		{
			CModel* pModel = (CModel*)pAttr;
			int PaletteIdx = pModel->ShaderVars.FindIndex(sidJointPalette);
			CMatrixPtrArray* pPalette;
			if (PaletteIdx == INVALID_INDEX)
			{
				CShaderVar& Var = pModel->ShaderVars.Add(sidJointPalette);
				Var.SetName(sidJointPalette);
				pPalette = &Var.Value.New<CMatrixPtrArray>();
				pPalette->SetGrowSize(1);
				Var.Bind(*pModel->Material->GetShader());
			}
			else pPalette = &pModel->ShaderVars.ValueAtIndex(PaletteIdx).Value.GetValue<CMatrixPtrArray>();

			if (!pModel->BoneIndices.Size())
				pPalette->At(Index) = &SkinMatrix;
			else
				for (int j = 0; j < pModel->BoneIndices.Size(); ++j)
					if (pModel->BoneIndices[j] == Index)
					{
						pPalette->At(j) = &SkinMatrix;
						break;
					}
		}
	}

	OK;
}
//---------------------------------------------------------------------

//!!!must be called recursively if only root node is detached!
//mb OnRemove() { DetachBones(); foreach Child { DetachBones(); } }
//and no recursion is needed with OnRemove
void CBone::OnRemove()
{
	CBone* pRootBone = GetRootBone();
	CSceneNode* pModelNode = pRootBone->pNode->GetParent() ? pRootBone->pNode->GetParent() : pRootBone->pNode;

	static CStrID sidJointPalette("JointPalette");

	// Find all models in model node and clear matrix pointers in a JointPalette shader var to const identity matrix
	for (DWORD i = 0; i < pModelNode->GetAttrCount(); ++i)
	{
		CSceneNodeAttr* pAttr = pModelNode->GetAttr(i);
		if (pAttr->IsA(CModel::RTTI) &&
			(((CModel*)pAttr)->FeatureFlags & RenderSrv->GetFeatureFlagSkinned()) &&
			((CModel*)pAttr)->Material.isvalid())
		{
			CModel* pModel = (CModel*)pAttr;
			int PaletteIdx = pModel->ShaderVars.FindIndex(sidJointPalette);
			if (PaletteIdx == INVALID_INDEX) continue;
			CMatrixPtrArray* pPalette = &pModel->ShaderVars.ValueAtIndex(PaletteIdx).Value.GetValue<CMatrixPtrArray>();

			if (!pModel->BoneIndices.Size())
				pPalette->At(Index) = &matrix44::identity;
			else
				for (int j = 0; j < pModel->BoneIndices.Size(); ++j)
					if (pModel->BoneIndices[j] == Index)
					{
						pPalette->At(j) = &matrix44::identity;
						break;
					}
		}
	}
}
//---------------------------------------------------------------------

void CBone::Update()
{
	if (pNode->IsWorldMatrixChanged())
		SkinMatrix.mult2_simple(InvBindPose, pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

CBone* CBone::GetParentBone() const
{
	n_assert_dbg(pNode);
	if (Flags.Is(Bone_Root) || !pNode->GetParent()) return NULL;
	return pNode->GetParent()->FindFirstAttr<CBone>();
}
//---------------------------------------------------------------------

CBone* CBone::GetRootBone()
{
	n_assert_dbg(pNode);
	if (Flags.Is(Bone_Root)) return this;

	CBone* pCurrBone = this;
	do pCurrBone = pCurrBone->GetParentBone();
	while (pCurrBone && !pCurrBone->Flags.Is(Bone_Root));

	return pCurrBone;
}
//---------------------------------------------------------------------

}