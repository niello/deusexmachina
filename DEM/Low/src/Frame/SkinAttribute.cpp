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
	SAFE_FREE_ALIGNED(_pSkinPalette);
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
				_SkinInfoUID = DataReader.Read<CStrID>();
				break;
			}
			case 'RSPH':
			{
				_RootSearchPath = DataReader.Read<CString>();
				break;
			}
			case 'ACBN':
			{
				_Flags.SetTo(Skin_AutocreateBones, DataReader.Read<bool>());
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
	const UPTR BoneCount = _SkinInfo->GetBoneCount();
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		auto& pBoneNode = _BoneNodes[i].pNode;
		if (pBoneNode) continue;

		const auto& BoneInfo = _SkinInfo->GetBoneInfo(i);
		if (BoneInfo.ParentIndex != ParentIndex) continue;

		pBoneNode = ParentNode.GetChild(BoneInfo.ID);
		if (!pBoneNode && _Flags.Is(Skin_AutocreateBones))
			pBoneNode = ParentNode.CreateChild(BoneInfo.ID);

		if (pBoneNode)
		{
			// Set skinned mesh into a bind pose initially
			matrix44 BindPoseLocal;
			_SkinInfo->GetInvBindPose(i).invert_simple(BindPoseLocal);
			if (ParentIndex != INVALID_INDEX)
				BindPoseLocal.mult_simple(_SkinInfo->GetInvBindPose(ParentIndex));
			pBoneNode->SetLocalTransform(BindPoseLocal);

			SetupBoneNodes(i, *pBoneNode);
		}
	}
}
//---------------------------------------------------------------------

bool CSkinAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	if (!_SkinInfo)
	{
		Resources::PResource Rsrc = ResMgr.RegisterResource<Render::CSkinInfo>(_SkinInfoUID);
		_SkinInfo = Rsrc->ValidateObject<Render::CSkinInfo>();
	}

	if (!_pNode || !_SkinInfo) FAIL;

	auto pRootParent = _pNode->FindNodeByPath(_RootSearchPath.CStr());
	if (!pRootParent) FAIL;

	const UPTR BoneCount = _SkinInfo->GetBoneCount();
	_pSkinPalette = static_cast<matrix44*>(n_malloc_aligned(BoneCount * sizeof(matrix44), 16));

	_BoneNodes.clear();
	_BoneNodes.resize(BoneCount);

	// Setup root bone(s) and recurse down the hierarchy
	SetupBoneNodes(INVALID_INDEX, *pRootParent);

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CSkinAttribute::Clone()
{
	PSkinAttribute ClonedAttr = n_new(CSkinAttribute);
	ClonedAttr->_RootSearchPath = _RootSearchPath;
	ClonedAttr->_SkinInfoUID = _SkinInfoUID;
	ClonedAttr->_Flags.SetTo(Skin_AutocreateBones, _Flags.Is(Skin_AutocreateBones));
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CSkinAttribute::UpdateAfterChildren(const vector3* pCOIArray, UPTR COICount)
{
	CNodeAttribute::UpdateAfterChildren(pCOIArray, COICount);

	if (!_SkinInfo || _BoneNodes.empty() || !_pSkinPalette) return;

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

}