#include "NodeAttrSkin.h"

#include <Render/SkinInfo.h>
#include <Scene/SceneNode.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Resources/ResourceCreator.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

// See CNodeAttrSkin::Initialize()
#define NOT_PROCESSED_NODE (pNode)

namespace Frame
{
__ImplementClass(Frame::CNodeAttrSkin, 'SKIN', Scene::CNodeAttribute);

CNodeAttrSkin::~CNodeAttrSkin()
{
	SAFE_DELETE_ARRAY(pBoneNodes);
	SAFE_FREE_ALIGNED(pSkinPalette);
}
//---------------------------------------------------------------------

bool CNodeAttrSkin::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'SKIF':
		{
			//???!!!store the whole URI in a file?!
			//???store resource locally for reloading?
			CString RsrcID = DataReader.Read<CString>();
			CStrID RUID = CStrID(CString("Scene:") + RsrcID.CStr() + ".skn"); //???Skins:?
			Resources::PResource Rsrc = ResourceMgr->RegisterResource<Render::CSkinInfo>(RUID);
			SkinInfo = Rsrc->ValidateObject<Render::CSkinInfo>();
			OK;
		}
		case 'ACBN':
		{
			Flags.SetTo(Skin_AutocreateBones, DataReader.Read<bool>());
			OK;
		}
		default: return CNodeAttribute::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CNodeAttrSkin::Clone()
{
	PNodeAttrSkin ClonedAttr = n_new(CNodeAttrSkin);
	ClonedAttr->SkinInfo = SkinInfo;
	ClonedAttr->Flags.SetTo(Skin_AutocreateBones, Flags.Is(Skin_AutocreateBones));
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

Scene::CSceneNode* CNodeAttrSkin::SetupBoneNode(UPTR BoneIndex)
{
	if (pBoneNodes[BoneIndex] == NOT_PROCESSED_NODE)
	{
		const Render::CBoneInfo& BoneInfo = SkinInfo->GetBoneInfo(BoneIndex);
		Scene::CSceneNode* pParentBoneNode = (BoneInfo.ParentIndex == INVALID_INDEX) ? pNode : SetupBoneNode(BoneInfo.ParentIndex);
		if (pParentBoneNode)
		{
			Scene::CSceneNode* pBoneNode = pParentBoneNode->GetChild(BoneInfo.ID);
			if (!pBoneNode && Flags.Is(Skin_AutocreateBones)) pBoneNode = pParentBoneNode->CreateChild(BoneInfo.ID);
			pBoneNodes[BoneIndex] = pBoneNode;

			// Set skinned mesh into a bind pose initially
			if (pBoneNode)
			{
				matrix44 BindPoseLocal;
				SkinInfo->GetInvBindPose(BoneIndex).invert_simple(BindPoseLocal);
				if (BoneInfo.ParentIndex != INVALID_INDEX)
					BindPoseLocal.mult_simple(SkinInfo->GetInvBindPose(BoneInfo.ParentIndex));
				pBoneNode->SetLocalTransform(BindPoseLocal);
			}
		}
		else pBoneNodes[BoneIndex] = nullptr;
	}
	return pBoneNodes[BoneIndex];
}
//---------------------------------------------------------------------

bool CNodeAttrSkin::Initialize()
{
	if (!pNode || SkinInfo.IsNullPtr()) FAIL;

	UPTR BoneCount = SkinInfo->GetBoneCount();
	pSkinPalette = (matrix44*)n_malloc_aligned(BoneCount * sizeof(matrix44), 16);
	pBoneNodes = n_new_array(Scene::CSceneNode*, BoneCount);

	// Here we fill an array of scene nodes associated with bones of this skin instance. nullptr values
	// are valid for the case when bone doesn't exist and must not be autocreated. To make distinstion
	// between nullptr and not processed nodes we fill not processed ones with the value of pNode (skin
	// owner). No bone can use this node, so this value means 'not processed yet'. Hacky but convenient.
	// Since processing is recursive and random-access, we must clear array in a first pass and then
	// process it in a second pass.

	for (UPTR i = 0; i < BoneCount; ++i)
		pBoneNodes[i] = NOT_PROCESSED_NODE;

	for (UPTR i = 0; i < BoneCount; ++i)
		SetupBoneNode(i);

	OK;
}
//---------------------------------------------------------------------

void CNodeAttrSkin::Update(const vector3* pCOIArray, UPTR COICount)
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

Render::CSkinInfo* CNodeAttrSkin::GetSkinInfo() const
{
	return SkinInfo;
}
//---------------------------------------------------------------------

}