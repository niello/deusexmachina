#include "Skin.h"

#include <Render/SkinInfo.h>
#include <Scene/SceneNode.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CSkin, 'SKIN', Scene::CNodeAttribute);

bool CSkin::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'SKIF':
		{
			//???!!!store the whole URI in a file?!
			CString RsrcID = DataReader.Read<CString>();
			CStrID RsrcURI = CStrID(CString("Scene:") + RsrcID.CStr() + ".skn"); //???Skins:?
			Resources::PResource Rsrc = ResourceMgr->RegisterResource(RsrcURI);
			if (!Rsrc->IsLoaded())
			{
				Resources::PResourceLoader Loader = Rsrc->GetLoader();
				if (Loader.IsNullPtr())
					Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CSkinInfo>(PathUtils::GetExtension(RsrcURI.CStr()));
				ResourceMgr->LoadResourceSync(*Rsrc, *Loader);
				n_assert(Rsrc->IsLoaded());
			}
			SkinInfo = Rsrc->GetObject<Render::CSkinInfo>();
			OK;
		}
		case 'ACBN':
		{
			Flags.SetTo(AutocreateBones, DataReader.Read<bool>());
			OK;
		}
		default: return CNodeAttribute::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

/*
bool CSkin::OnAttachToNode(Scene::CSceneNode* pSceneNode)
{
	if (!CNodeAttribute::OnAttachToNode(pSceneNode)) FAIL;

	SkinMatrix.ident();

	// Since SkinMatrix is Identity and node position doesn't change, it is not necessary:
	// To undo inv. bind pose in initial state
	pNode->SetLocalTransform(BindPoseLocal);

	BindPoseLocal.ToMatrix(BindPoseWorld);

	CBone* pParentBone = GetParentBone();
	if (pParentBone) BindPoseWorld.mult_simple(pParentBone->GetBindPoseMatrix());

	BindPoseWorld.invert_simple(InvBindPose);

	CBone* pRootBone = GetRootBone();
	Scene::CSceneNode* pModelNode = pRootBone->pNode->GetParent() ? pRootBone->pNode->GetParent() : pRootBone->pNode;

	static CStrID sidJointPalette("JointPalette");
	// Find all models in model node and setup matrix pointers in a JointPalette shader var
	for (DWORD i = 0; i < pModelNode->GetAttributeCount(); ++i)
	{
		CNodeAttribute* pAttr = pModelNode->GetAttribute(i);
		if (pAttr->IsA(CModel::RTTI) &&
			(((CModel*)pAttr)->FeatureFlags & RenderSrv->GetFeatureFlagSkinned()) &&
			((CModel*)pAttr)->Material.IsValid())
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
			else pPalette = &pModel->ShaderVars.ValueAt(PaletteIdx).Value.GetValue<CMatrixPtrArray>();

			if (!pModel->BoneIndices.GetCount())
				pPalette->At(Index) = &SkinMatrix;
			else
				for (DWORD j = 0; j < pModel->BoneIndices.GetCount(); ++j)
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
*/

void CSkin::Update(const vector3* pCOIArray, UPTR COICount)
{
	CNodeAttribute::Update(pCOIArray, COICount);

	if (SkinInfo.IsNullPtr() || !pBoneNodes || !pSkinPalette) return;

	UPTR BoneCount = SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		Scene::CSceneNode* pBoneNode = pBoneNodes[i];
		if (pBoneNode && pBoneNode->IsWorldMatrixChanged())
			pSkinPalette[i].mult2_simple(SkinInfo->GetInvBindPose(i), pBoneNode->GetWorldMatrix());
	}
}
//---------------------------------------------------------------------

}