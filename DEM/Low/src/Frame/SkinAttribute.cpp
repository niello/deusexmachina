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
				SkinInfoUID = DataReader.Read<CStrID>();
				break;
			}
			case 'RSPH':
			{
				RootSearchPath = DataReader.Read<CString>();
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

void CSkinAttribute::SetupBoneNodes(UPTR ParentIndex, Scene::CSceneNode& ParentNode)
{
	const UPTR BoneCount = SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		auto& pBoneNode = BoneNodes[i].pNode;
		if (pBoneNode) continue;

		const auto& BoneInfo = SkinInfo->GetBoneInfo(i);
		if (BoneInfo.ParentIndex != ParentIndex) continue;

		pBoneNode = ParentNode.GetChild(BoneInfo.ID);
		if (!pBoneNode && Flags.Is(Skin_AutocreateBones))
			pBoneNode = ParentNode.CreateChild(BoneInfo.ID);

		if (pBoneNode)
		{
			// Set skinned mesh into a bind pose initially
			matrix44 BindPoseLocal;
			SkinInfo->GetInvBindPose(i).invert_simple(BindPoseLocal);
			if (ParentIndex != INVALID_INDEX)
				BindPoseLocal.mult_simple(SkinInfo->GetInvBindPose(ParentIndex));
			pBoneNode->SetLocalTransform(BindPoseLocal);

			SetupBoneNodes(i, *pBoneNode);
		}
	}
}
//---------------------------------------------------------------------

bool CSkinAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	Resources::PResource Rsrc = ResMgr.RegisterResource<Render::CSkinInfo>(SkinInfoUID);
	SkinInfo = Rsrc->ValidateObject<Render::CSkinInfo>();

	if (!pNode || !SkinInfo) FAIL;

	auto pRootParent = pNode->FindNodeByPath(RootSearchPath.CStr());
	if (!pRootParent) FAIL;

	const UPTR BoneCount = SkinInfo->GetBoneCount();
	pSkinPalette = static_cast<matrix44*>(n_malloc_aligned(BoneCount * sizeof(matrix44), 16));

	BoneNodes.clear();
	BoneNodes.resize(BoneCount);

	// Setup root bone(s) and recurse down the hierarchy
	SetupBoneNodes(INVALID_INDEX, *pRootParent);

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CSkinAttribute::Clone()
{
	PSkinAttribute ClonedAttr = n_new(CSkinAttribute);
	ClonedAttr->RootSearchPath = RootSearchPath;
	ClonedAttr->SkinInfoUID = SkinInfoUID;
	ClonedAttr->Flags.SetTo(Skin_AutocreateBones, Flags.Is(Skin_AutocreateBones));
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CSkinAttribute::Update(const vector3* pCOIArray, UPTR COICount)
{
	CNodeAttribute::Update(pCOIArray, COICount);

	if (!SkinInfo || BoneNodes.empty() || !pSkinPalette) return;

	const UPTR BoneCount = SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		Scene::CSceneNode* pBoneNode = BoneNodes[i].pNode;
		if (pBoneNode && pBoneNode->GetTransformVersion() != BoneNodes[i].LastTransformVersion)
		{
			pSkinPalette[i].mult2_simple(SkinInfo->GetInvBindPose(i), pBoneNode->GetWorldMatrix());
			BoneNodes[i].LastTransformVersion = pBoneNode->GetTransformVersion();
		}
	}
}
//---------------------------------------------------------------------

}