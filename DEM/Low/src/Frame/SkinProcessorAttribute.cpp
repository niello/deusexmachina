#include "SkinProcessorAttribute.h"
#include <Render/SkinInfo.h>
#include <Scene/SceneNode.h>

namespace Frame
{

CSkinPalette::CSkinPalette(Render::PSkinInfo SkinInfo, Scene::CSceneNode& RootParentNode, bool CreateMissingBones)
	: _SkinInfo(std::move(SkinInfo))
{
	if (!_SkinInfo) return;

	const UPTR BoneCount = _SkinInfo->GetBoneCount();
	_BoneNodes = std::make_unique<CBoneInfo[]>(BoneCount);

	// Allocate aligned to 16 for faster transfer to VRAM
	_pSkinPalette = static_cast<matrix44*>(n_malloc_aligned(BoneCount * sizeof(matrix44), 16));

	// Setup root bone(s) and recurse down the hierarchy
	SetupBoneNodes(INVALID_INDEX, RootParentNode, CreateMissingBones);
}
//---------------------------------------------------------------------

CSkinPalette::~CSkinPalette()
{
	SAFE_FREE_ALIGNED(_pSkinPalette); // TODO: use aligned unique array ptr?
}
//---------------------------------------------------------------------

void CSkinPalette::SetupBoneNodes(UPTR ParentIndex, Scene::CSceneNode& ParentNode, bool CreateMissingBones)
{
	const UPTR BoneCount = _SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		// Reset palette just in case some nodes are not found
		_pSkinPalette[i].ident();

		auto& pBoneNode = _BoneNodes[i].pNode;
		if (pBoneNode) continue;

		const auto& BoneInfo = _SkinInfo->GetBoneInfo(i);
		if (BoneInfo.ParentIndex != ParentIndex) continue;

		pBoneNode = ParentNode.GetChild(BoneInfo.ID);
		if (!pBoneNode && CreateMissingBones)
			pBoneNode = ParentNode.CreateChild(BoneInfo.ID);

		if (pBoneNode)
		{
			// Set skinned mesh into a bind pose initially
			matrix44 BindPoseLocal;
			_SkinInfo->GetInvBindPose(i).invert_simple(BindPoseLocal);
			if (ParentIndex != INVALID_INDEX)
				BindPoseLocal.mult_simple(_SkinInfo->GetInvBindPose(ParentIndex));
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
			_pSkinPalette[i].mult2_simple(_SkinInfo->GetInvBindPose(i), pBoneNode->GetWorldMatrix());
			_BoneNodes[i].LastTransformVersion = pBoneNode->GetTransformVersion();
		}
	}
}
//---------------------------------------------------------------------

CSkinProcessorAttribute::~CSkinProcessorAttribute() = default;
//---------------------------------------------------------------------

void CSkinProcessorAttribute::UpdateAfterChildren(const vector3* pCOIArray, UPTR COICount)
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
	if (!SkinInfo) return nullptr;

	auto It = std::lower_bound(_Palettes.begin(), _Palettes.end(), SkinInfo.Get(),
		[](const PSkinPalette& Palette, const Render::CSkinInfo* pSkin) { return Palette->GetSkinInfo() < pSkin; });
	if (It != _Palettes.end() && (*It)->GetSkinInfo() == SkinInfo)
	{
		if (CreateMissingBones)
		{
			// TODO: check if some bones are unbound, create them and rebind their subtrees
		}

		return *It;
	}

	PSkinPalette Palette = n_new(CSkinPalette)(SkinInfo, *_pNode, CreateMissingBones);
	_Palettes.insert(It, Palette);
	return Palette;
}
//---------------------------------------------------------------------

}
