#pragma once
#ifndef __DEM_L1_RENDER_CDLOD_DATA_H__
#define __DEM_L1_RENDER_CDLOD_DATA_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Data/FixedArray.h>

// CDLOD heightfield-based terrain rendering data with settings and precalculated aux data

namespace Resources
{
	class CCDLODDataLoader;
}

namespace Render
{

class CCDLODData: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

protected:

	struct CMinMaxMap
	{
		UPTR	PatchesW;
		UPTR	PatchesH;
		I16*	pData;
	};

	PTexture				HeightMap;
	PTexture				NormalMap;

	I16*					pMinMaxData;
	CFixedArray<CMinMaxMap>	MinMaxMaps;

	U32						HFWidth;
	U32						HFHeight;
	U32						PatchSize;
	U32						TopPatchCountW;
	U32						TopPatchCountH;
	U32						LODCount;
	float					VerticalScale;
	CAABB					Box;

	friend class Resources::CCDLODDataLoader;

public:

	CCDLODData(): pMinMaxData(NULL) {}
	//virtual ~CCDLODData();

	virtual bool		IsResourceValid() const { return HeightMap.IsValidPtr(); }
	U32					GetHeightMapWidth() const { return HFWidth; }
	U32					GetHeightMapHeight() const { return HFHeight; }
	U32					GetPatchSize() const { return PatchSize; }
	U32					GetTopPatchCountW() const { return TopPatchCountW; }
	U32					GetTopPatchCountH() const { return TopPatchCountH; }
	U32					GetLODCount() const { return LODCount; }
	float				GetVerticalScale() const { return VerticalScale; }
	const CAABB&		GetAABB() const { return Box; }
	Render::CTexture*	GetHeightMap() const { return HeightMap.GetUnsafe(); }
	Render::CTexture*	GetNormalMap() const { return NormalMap.GetUnsafe(); }
	void				GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, I16& MinY, I16& MaxY) const;
	bool				HasNode(UPTR X, UPTR Z, UPTR LOD) const;
};

typedef Ptr<CCDLODData> PCDLODData;

inline void CCDLODData::GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, I16& MinY, I16& MaxY) const
{
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	n_assert_dbg(X < MMMap.PatchesW && Z < MMMap.PatchesH);
	const I16* pMMRec = MMMap.pData + ((Z * MMMap.PatchesW + X) << 1);
	MinY = *pMMRec;
	MaxY = *(pMMRec + 1);
}
//---------------------------------------------------------------------

inline bool CCDLODData::HasNode(UPTR X, UPTR Z, UPTR LOD) const
{
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	return X < MMMap.PatchesW && Z < MMMap.PatchesH;
}
//---------------------------------------------------------------------

}

#endif
