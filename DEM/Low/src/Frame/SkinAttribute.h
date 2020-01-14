#pragma once
#include <Scene/NodeAttribute.h>
#include <Math/Matrix44.h>
#include <Data/String.h>

// Skin attribute adds a skinned rendering capability to sibling models, providing the skin palette for them

namespace Render
{
	typedef Ptr<class CSkinInfo> PSkinInfo;
};

namespace Frame
{

class CSkinAttribute : public Scene::CNodeAttribute
{
	FACTORY_CLASS_DECL;

protected:

	enum // extends Scene::CNodeAttribute enum
	{
		// Active
		// WorldMatrixChanged
		Skin_AutocreateBones = 0x04
	};

	struct CBoneInfo
	{
		Scene::CSceneNode* pNode = nullptr; //???FIXME: strong refs? or weak, but not raw!
		U32 LastTransformVersion = 0;
	};

	CString                RootSearchPath;
	CStrID                 SkinInfoUID;
	Render::PSkinInfo      SkinInfo;
	matrix44*              pSkinPalette = nullptr;
	std::vector<CBoneInfo> BoneNodes;

	void SetupBoneNodes(UPTR ParentIndex, Scene::CSceneNode& ParentNode);

	//!!!if no joint palette, model uses all skin palette as a variable, copying directly,
	//with palette it copies only a part! catch redundant sets
	//!!!may allocate joint palette matrix44 array inside a model!

public:

	virtual ~CSkinAttribute() override;

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual bool                  ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual void                  UpdateAfterChildren(const vector3* pCOIArray, UPTR COICount) override;

	Render::CSkinInfo*            GetSkinInfo() const { return SkinInfo.Get(); }
	const matrix44*               GetSkinPalette() const { return pSkinPalette; }
};

typedef Ptr<CSkinAttribute> PSkinAttribute;

}
