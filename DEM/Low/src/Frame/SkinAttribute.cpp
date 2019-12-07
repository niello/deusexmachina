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
				SkinInfoID = DataReader.Read<CStrID>();
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

void CSkinAttribute::SetupBoneNodes(UPTR ParentIndex, Scene::CSceneNode* pParentNode)
{
	const UPTR BoneCount = SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		const auto& BoneInfo = SkinInfo->GetBoneInfo(i);

		if (BoneInfo.ParentIndex != ParentIndex) continue;

		auto& pBoneNode = BoneNodes[i];

		pBoneNode = pParentNode->GetChild(BoneInfo.ID);
		if (!pBoneNode && Flags.Is(Skin_AutocreateBones))
			pBoneNode = pParentNode->CreateChild(BoneInfo.ID);

		// Set skinned mesh into a bind pose initially
		if (pBoneNode)
		{
			matrix44 BindPoseLocal;
			SkinInfo->GetInvBindPose(i).invert_simple(BindPoseLocal);
			if (ParentIndex != INVALID_INDEX)
				BindPoseLocal.mult_simple(SkinInfo->GetInvBindPose(ParentIndex));
			pBoneNode->SetLocalTransform(BindPoseLocal);

			SetupBoneNodes(i, pBoneNode);
		}
	}
}
//---------------------------------------------------------------------

bool CSkinAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	Resources::PResource Rsrc = ResMgr.RegisterResource<Render::CSkinInfo>(SkinInfoID);
	SkinInfo = Rsrc->ValidateObject<Render::CSkinInfo>();

	if (!pNode || !SkinInfo) FAIL;

	auto pRootParent = pNode->FindNodeByPath(RootSearchPath.CStr());
	if (!pRootParent) FAIL;

	const UPTR BoneCount = SkinInfo->GetBoneCount();
	pSkinPalette = static_cast<matrix44*>(n_malloc_aligned(BoneCount * sizeof(matrix44), 16));

	BoneNodes.clear();
	BoneNodes.resize(BoneCount, nullptr);

	// Setup root bone(s) and recurse down the hierarchy
	SetupBoneNodes(INVALID_INDEX, pRootParent);

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

	if (!SkinInfo || BoneNodes.empty() || !pSkinPalette) return;

	const UPTR BoneCount = SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		const Scene::CSceneNode* pBoneNode = BoneNodes[i];
		if (pBoneNode && pBoneNode->IsWorldMatrixChanged())
			pSkinPalette[i].mult2_simple(SkinInfo->GetInvBindPose(i), pBoneNode->GetWorldMatrix());
	}
}
//---------------------------------------------------------------------

}