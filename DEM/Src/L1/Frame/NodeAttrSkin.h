#pragma once
#ifndef __DEM_L1_FRAME_SKIN_H__
#define __DEM_L1_FRAME_SKIN_H__

#include <Scene/NodeAttribute.h>
#include <Math/Matrix44.h>

// Skin attribute adds a skinned rendering capability to sibling models, providing the skin palette for them

//!!!need post-load instance initialization to map/create bones and allocate a palette!

namespace Render
{
	typedef Ptr<class CSkinInfo> PSkinInfo;
};

namespace Frame
{

class CNodeAttrSkin: public Scene::CNodeAttribute
{
	__DeclareClass(CNodeAttrSkin);

protected:

	enum // extends Scene::CNodeAttribute enum
	{
		// Active
		// WorldMatrixChanged
		AutocreateBones = 0x04
	};

	Render::PSkinInfo	SkinInfo;
	matrix44*			pSkinPalette; //!!!allocate aligned skin palette!
	Scene::CSceneNode**	pBoneNodes; //???strong refs?

	//!!!if no joint palette, model uses all skin palette as a variable, copying directly,
	//with palette it copies only a part! catch redundant sets

public:

	CNodeAttrSkin(): pSkinPalette(NULL), pBoneNodes(NULL) {}

	virtual bool		LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual void		Update(const vector3* pCOIArray, UPTR COICount);

	Render::PSkinInfo	GetSkinInfo() const { return SkinInfo; }
	const matrix44*		GetSkinPalette() const { return pSkinPalette; }
};

typedef Ptr<CNodeAttrSkin> PSkin;

}

#endif
