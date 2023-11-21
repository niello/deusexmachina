#include "SkinProcessorAttribute.h"
#include <Render/SkinInfo.h>
#include <Scene/SceneNode.h>

namespace Frame
{

CSkinPalette::~CSkinPalette()
{
	SAFE_FREE_ALIGNED(_pSkinPalette); // TODO: use aligned unique array ptr?
}
//---------------------------------------------------------------------

void CSkinPalette::SetSkinInfo(Render::PSkinInfo SkinInfo)
{
	if (!SkinInfo || _SkinInfo == SkinInfo) return;

	const UPTR BoneCount = SkinInfo->GetBoneCount();
	const UPTR PrevBoneCount = _SkinInfo ? _SkinInfo->GetBoneCount() : 0;

	_SkinInfo = std::move(SkinInfo);

	if (BoneCount != PrevBoneCount)
	{
		SAFE_FREE_ALIGNED(_pSkinPalette); // TODO: use aligned unique array ptr?

		// Allocate aligned to 16 for faster transfer to VRAM
		_pSkinPalette = static_cast<rtm::matrix3x4f*>(n_malloc_aligned(BoneCount * sizeof(rtm::matrix3x4f), alignof(rtm::matrix3x4f)));
		_BoneNodes = std::make_unique<CBoneInfo[]>(BoneCount);
	}
}
//---------------------------------------------------------------------

void CSkinPalette::SetupBoneNodes(UPTR ParentIndex, Scene::CSceneNode& ParentNode, bool CreateMissingBones)
{
	const UPTR BoneCount = _SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		// Skip if already bound
		auto& pBoneNode = _BoneNodes[i].pNode;
		if (pBoneNode) continue;

		// Reset palette just in case some nodes are not found
		_pSkinPalette[i] = rtm::matrix_identity();

		const auto& BoneInfo = _SkinInfo->GetBoneInfo(i);
		if (BoneInfo.ParentIndex != ParentIndex) continue;

		pBoneNode = ParentNode.GetChild(BoneInfo.ID);
		if (!pBoneNode && CreateMissingBones)
			pBoneNode = ParentNode.CreateChild(BoneInfo.ID);

		if (pBoneNode)
		{
			// Set skinned mesh into a bind pose initially
			rtm::matrix3x4f BindPoseLocal = rtm::matrix_inverse(_SkinInfo->GetInvBindPose(i));
			if (ParentIndex != INVALID_INDEX)
				BindPoseLocal = rtm::matrix_mul(BindPoseLocal, _SkinInfo->GetInvBindPose(ParentIndex));
			pBoneNode->SetLocalTransform(BindPoseLocal);

			SetupBoneNodes(i, *pBoneNode, CreateMissingBones);
		}
	}
}
//---------------------------------------------------------------------

void CSkinPalette::Update()
{
	if (!_SkinInfo || !_BoneNodes || !_pSkinPalette) return;

	const UPTR BoneCount = _SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		const Scene::CSceneNode* pBoneNode = _BoneNodes[i].pNode;
		if (pBoneNode && pBoneNode->GetTransformVersion() != _BoneNodes[i].LastTransformVersion)
		{
			_pSkinPalette[i] = rtm::matrix_mul(_SkinInfo->GetInvBindPose(i), pBoneNode->GetWorldMatrix());
			_BoneNodes[i].LastTransformVersion = pBoneNode->GetTransformVersion();
		}
	}
}
//---------------------------------------------------------------------

CSkinProcessorAttribute::~CSkinProcessorAttribute() = default;
//---------------------------------------------------------------------

void CSkinProcessorAttribute::UpdateAfterChildren(const rtm::vector4f* pCOIArray, UPTR COICount)
{
	CNodeAttribute::UpdateAfterChildren(pCOIArray, COICount);

	// Update palettes currently used by skinned meshes
	for (auto& Palette : _Palettes)
		if (Palette->GetRefCount() > 1)
			Palette->Update();
}
//---------------------------------------------------------------------

PSkinPalette CSkinProcessorAttribute::GetSkinPalette(Render::PSkinInfo SkinInfo, bool CreateMissingBones)
{
	if (!SkinInfo || !_pNode) return nullptr;

	// Try to find an existing palette for this skin
	auto It = _SkinToPalette.find(SkinInfo);
	if (It == _SkinToPalette.end())
	{
		// Try to find a palette for a fully compatible skin. This is highly likely that we find it because
		// a skeleton is shared between skins and they may be equal even if exported to different assets.
		auto It2 = _Palettes.begin();
		for (; It2 != _Palettes.end(); ++It2)
		{
			const UPTR MatchLength = SkinInfo->GetBoneMatchingLength(*(*It2)->GetSkinInfo());
			if (MatchLength == SkinInfo->GetBoneCount())
			{
				// We can reuse an existing palette
				break;
			}
			else if (MatchLength == (*It2)->GetSkinInfo()->GetBoneCount())
			{
				// We can extend an existing palette
				(*It2)->SetSkinInfo(SkinInfo);
				break;
			}
		}

		if (It2 != _Palettes.end())
		{
			// Explicitly associate the skin with the matching palette so that we immediately find it instead of matching again
			It = _SkinToPalette.emplace(std::move(SkinInfo), (*It2).Get()).first;
		}
		else
		{
			// If not found, register a new skin palette
			_Palettes.push_back(PSkinPalette(n_new(CSkinPalette)));
			_Palettes.back()->SetSkinInfo(SkinInfo);
			It = _SkinToPalette.emplace(std::move(SkinInfo), _Palettes.back().Get()).first;
		}
	}

	// Bind new bones and create missing ones if we hadn't yet
	It->second->SetupBoneNodes(INVALID_INDEX, *_pNode, CreateMissingBones);

	return It->second;
}
//---------------------------------------------------------------------

}
