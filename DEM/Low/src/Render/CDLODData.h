#pragma once
#include <Core/Object.h>
#include <Data/FixedArray.h>
#include <Math/AABB.h>
#include <array>

// CDLOD heightfield-based terrain rendering data with settings and precalculated aux data

namespace Resources
{
	class CCDLODDataLoader;
}

namespace Render
{

class CCDLODData: public ::Core::CObject
{
	RTTI_CLASS_DECL(Render::CCDLODData, ::Core::CObject);

protected:

	struct CMinMaxMap
	{
		UPTR	PatchesW;
		UPTR	PatchesH;
		I16*	pData;
	};

	std::unique_ptr<I16[]>  pMinMaxData = nullptr;
	CFixedArray<CMinMaxMap>	MinMaxMaps;

	U32						HFWidth;
	U32						HFHeight;
	U32						PatchSize;
	U32						LODCount;
	std::array<float, 4>	SplatMapUVCoeffs; // xy - scale, zw - offset
	CAABB					Box;

	friend class Resources::CCDLODDataLoader;

public:

#pragma pack(push, 1)
	struct CHeader_0_2_0_0
	{
		U32                  HFWidth;
		U32                  HFHeight;
		U32                  PatchSize;
		U32                  LODCount;
		std::array<float, 4> SplatMapUVCoeffs;
		U32                  MinMaxDataCount;
	};
#pragma pack(pop)

	U32					GetHeightMapWidth() const { return HFWidth; }
	U32					GetHeightMapHeight() const { return HFHeight; }
	U32					GetPatchSize() const { return PatchSize; }
	U32					GetLODCount() const { return LODCount; }
	const CAABB&		GetAABB() const { return Box; }
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
