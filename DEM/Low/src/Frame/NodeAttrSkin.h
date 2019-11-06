#pragma once
#include <Scene/NodeAttribute.h>
#include <Math/Matrix44.h>

// Skin attribute adds a skinned rendering capability to sibling models, providing the skin palette for them

namespace Render
{
	typedef Ptr<class CSkinInfo> PSkinInfo;
};

namespace Frame
{

class CNodeAttrSkin : public Scene::CNodeAttribute
{
	__DeclareClass(CNodeAttrSkin);

protected:

	enum // extends Scene::CNodeAttribute enum
	{
		// Active
		// WorldMatrixChanged
		Skin_AutocreateBones = 0x04
	};

	CStrID              SkinInfoID;
	Render::PSkinInfo   SkinInfo;
	matrix44*           pSkinPalette = nullptr;
	Scene::CSceneNode** pBoneNodes = nullptr; //???strong refs?

	Scene::CSceneNode* SetupBoneNode(UPTR BoneIndex);

	//!!!if no joint palette, model uses all skin palette as a variable, copying directly,
	//with palette it copies only a part! catch redundant sets
	//!!!may allocate joint palette matrix44 array inside a model!

public:

	virtual ~CNodeAttrSkin() override;

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count);
	virtual bool                  ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual Scene::PNodeAttribute Clone();
	virtual void                  Update(const vector3* pCOIArray, UPTR COICount);

	Render::CSkinInfo*            GetSkinInfo() const { return SkinInfo.Get(); }
	const matrix44*               GetSkinPalette() const { return pSkinPalette; }
};

typedef Ptr<CNodeAttrSkin> PNodeAttrSkin;

}
