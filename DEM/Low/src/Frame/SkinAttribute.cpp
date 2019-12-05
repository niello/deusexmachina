#include "SkinAttribute.h"
#include <Render/SkinInfo.h>
#include <Scene/SceneNode.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CSkinAttribute, 'SKIN', Scene::CNodeAttribute);

CSkinAttribute::~CSkinAttribute()
{
	SAFE_DELETE_ARRAY(pBoneNodes);
	SAFE_FREE_ALIGNED(pSkinPalette);
}
//---------------------------------------------------------------------

bool CSkinAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'SKIF':
			{
				SkinInfoID = DataReader.Read<CStrID>();
				break;
			}
			case 'ACBN':
			{
				Flags.SetTo(Skin_AutocreateBones, DataReader.Read<bool>());
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

#define NOT_PROCESSED_NODE (pNode)

Scene::CSceneNode* CSkinAttribute::SetupBoneNode(UPTR BoneIndex)
{
	// FIXME: if no parent node, must search by ID from the object root node!

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

bool CSkinAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	Resources::PResource Rsrc = ResMgr.RegisterResource<Render::CSkinInfo>(SkinInfoID);
	SkinInfo = Rsrc->ValidateObject<Render::CSkinInfo>();

	if (!pNode || !SkinInfo) FAIL;

	const UPTR BoneCount = SkinInfo->GetBoneCount();
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

Scene::PNodeAttribute CSkinAttribute::Clone()
{
	PSkinAttribute ClonedAttr = n_new(CSkinAttribute);
	ClonedAttr->SkinInfo = SkinInfo;
	ClonedAttr->Flags.SetTo(Skin_AutocreateBones, Flags.Is(Skin_AutocreateBones));
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

void CSkinAttribute::Update(const vector3* pCOIArray, UPTR COICount)
{
	CNodeAttribute::Update(pCOIArray, COICount);

	if (SkinInfo.IsNullPtr() || !pBoneNodes || !pSkinPalette) return;

	const UPTR BoneCount = SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		Scene::CSceneNode* pBoneNode = pBoneNodes[i];
		if (pBoneNode && pBoneNode->IsWorldMatrixChanged())
			pSkinPalette[i].mult2_simple(SkinInfo->GetInvBindPose(i), pBoneNode->GetWorldMatrix());
	}
}
//---------------------------------------------------------------------

}