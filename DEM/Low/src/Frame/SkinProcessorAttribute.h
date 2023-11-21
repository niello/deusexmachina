#pragma once
#include <Scene/NodeAttribute.h>
#include <Math/Matrix44.h>
#include <map>

// Dynamically created attribute that provides skin palettes for different skins based on the underlying node hierarchy

namespace Render
{
	typedef Ptr<class CSkinInfo> PSkinInfo;
};

namespace Frame
{

class CSkinPalette : public Data::CRefCounted
{
protected:

	struct CBoneInfo
	{
		Scene::CSceneNode* pNode = nullptr; //???FIXME: strong refs? or weak, but not raw!
		U32 LastTransformVersion = 0;
	};

	Render::PSkinInfo            _SkinInfo;
	matrix44*                    _pSkinPalette = nullptr;
	std::unique_ptr<CBoneInfo[]> _BoneNodes;

public:

	virtual ~CSkinPalette() override;

	void SetSkinInfo(Render::PSkinInfo SkinInfo);
	void SetupBoneNodes(UPTR ParentIndex, Scene::CSceneNode& ParentNode, bool CreateMissingBones);

	void Update();

	const Render::CSkinInfo* GetSkinInfo() const { return _SkinInfo.Get(); }
	const matrix44*          GetSkinPalette() const { return _pSkinPalette; }
};

using PSkinPalette = Ptr<CSkinPalette>;

class CSkinProcessorAttribute : public Scene::CNodeAttribute
{
	RTTI_CLASS_DECL(CSkinProcessorAttribute, Scene::CNodeAttribute);

protected:

	std::vector<PSkinPalette>                  _Palettes;
	std::map<Render::PSkinInfo, CSkinPalette*> _SkinToPalette;

	virtual void UpdateAfterChildren(const rtm::vector4f* pCOIArray, UPTR COICount) override;

public:

	virtual ~CSkinProcessorAttribute() override;

	virtual Scene::PNodeAttribute Clone() override { return nullptr; } // Created dynamically, not cloned

	PSkinPalette GetSkinPalette(Render::PSkinInfo SkinInfo, bool CreateMissingBones);
};

typedef Ptr<CSkinProcessorAttribute> PSkinProcessorAttribute;

}
