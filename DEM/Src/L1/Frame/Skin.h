#pragma once
#ifndef __DEM_L1_FRAME_SKIN_H__
#define __DEM_L1_FRAME_SKIN_H__

#include <Scene/NodeAttribute.h>
#include <Math/Matrix44.h>

// Skin attribute adds a skinned rendering capability to sibling models, providing the skin palette for them

namespace Render
{
	typedef Ptr<class CSkinInfo> PSkinInfo;
};

namespace Frame
{

class CSkin: public Scene::CNodeAttribute
{
	__DeclareClass(CSkin);

protected:

	Render::PSkinInfo	SkinInfo;
	matrix44*			pSkinPalette; //!!!allocate aligned skin palette!

	//!!!need resolved nodes - array indexed by bone index with nodes found by bone IDs from a SkinInfo!

	//!!!if no joint palette, model uses all skin palette as a variable, copying directly,
	//with palette it copies only a part! catch redundant sets

public:

	CSkin(): pSkinPalette(NULL) {}

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual void	Update(const vector3* pCOIArray, DWORD COICount);
};

typedef Ptr<CSkin> PSkin;

}

#endif
