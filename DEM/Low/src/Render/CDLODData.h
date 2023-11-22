#pragma once
#include <Core/Object.h>
#include <Data/FixedArray.h>
#include <Math/SIMDMath.h>
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

	Math::CAABB				Box;
	rtm::vector4f           BoundsMax;
	U32						HFWidth;
	U32						HFHeight;
	U32						PatchSize;
	U32						LODCount;
	float                   VerticalScale;
	std::array<float, 4>	SplatMapUVCoeffs; // xy - scale, zw - offset

	friend class Resources::CCDLODDataLoader;

public:

#pragma pack(push, 1)
	struct CHeader_0_2_0_0
	{
		U32                  HFWidth;
		U32                  HFHeight;
		U32                  PatchSize;
		U32                  LODCount;
		float                VerticalScale;
		std::array<float, 4> SplatMapUVCoeffs;
		U32                  MinMaxDataCount;
	};
#pragma pack(pop)

	U32					  GetHeightMapWidth() const { return HFWidth; }
	U32					  GetHeightMapHeight() const { return HFHeight; }
	U32					  GetPatchSize() const { return PatchSize; }
	U32					  GetLODCount() const { return LODCount; }
	float                 GetVerticalScale() const { return VerticalScale; }
	const auto&           GetSplatMapUVCoeffs() const { return SplatMapUVCoeffs;}
	const Math::CAABB&    GetAABB() const { return Box; }
	void				  GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, I16& MinY, I16& MaxY) const;
	void				  GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, float& MinY, float& MaxY) const;
	bool				  HasNode(UPTR X, UPTR Z, UPTR LOD) const;
	std::pair<bool, bool> GetChildExistence(UPTR X, UPTR Z, UPTR LOD) const;
	std::pair<UPTR, UPTR> GetLODSize(UPTR LOD) const;
	bool				  GetNodeAABB(UPTR X, UPTR Z, UPTR LOD, rtm::vector4f& BoxCenter, rtm::vector4f& BoxExtent) const;
	bool				  GetPatchAABB(UPTR X, UPTR Z, UPTR LOD, rtm::vector4f& BoxCenter, rtm::vector4f& BoxExtent) const;
	void				  ClampNodeToPatchAABB(rtm::vector4f& BoxCenter, rtm::vector4f& BoxExtent) const;
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

inline void CCDLODData::GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, float& MinY, float& MaxY) const
{
	I16 MinYShort, MaxYShort;
	GetMinMaxHeight(X, Z, LOD, MinYShort, MaxYShort);
	MinY = static_cast<float>(MinYShort) * VerticalScale;
	MaxY = static_cast<float>(MaxYShort) * VerticalScale;
}
//---------------------------------------------------------------------

inline bool CCDLODData::HasNode(UPTR X, UPTR Z, UPTR LOD) const
{
	const CMinMaxMap& MMMap = MinMaxMaps[LOD];
	return X < MMMap.PatchesW && Z < MMMap.PatchesH;
}
//---------------------------------------------------------------------

// Returns pair of HasRightChild and HasBottomChild
inline std::pair<bool, bool> CCDLODData::GetChildExistence(UPTR X, UPTR Z, UPTR LOD) const
{
	// Don't call this for leaves, they can't have children
	n_assert_dbg(LOD > 0);
	const CMinMaxMap& MMMap = MinMaxMaps[LOD - 1];
	return { ((X << 1) + 1 < MMMap.PatchesW), ((Z << 1) + 1 < MMMap.PatchesH) };
}
//---------------------------------------------------------------------

inline std::pair<UPTR, UPTR> CCDLODData::GetLODSize(UPTR LOD) const
{
	const CMinMaxMap& MMMap = MinMaxMaps[LOD - 1];
	return { MMMap.PatchesW, MMMap.PatchesH };
}
//---------------------------------------------------------------------

// Full AABB of the quadtree node
inline bool CCDLODData::GetNodeAABB(UPTR X, UPTR Z, UPTR LOD, rtm::vector4f& BoxCenter, rtm::vector4f& BoxExtent) const
{
	I16 MinY, MaxY;
	GetMinMaxHeight(X, Z, LOD, MinY, MaxY);

	// Node has no data
	if (MaxY < MinY) return false;

	const U32 NodeHalfSize = PatchSize << LOD >> 1; // PatchSize is always pow2, so instead of mul 0.5f we shift by 1
	const float MinYF = MinY * VerticalScale;
	const float MaxYF = MaxY * VerticalScale;

	BoxCenter = rtm::vector_set(static_cast<float>((2 * X + 1) * NodeHalfSize), (MaxYF + MinYF) * 0.5f, static_cast<float>((2 * Z + 1) * NodeHalfSize));
	BoxExtent = rtm::vector_set(static_cast<float>(NodeHalfSize), (MaxYF - MinYF) * 0.5f, static_cast<float>(NodeHalfSize));

	return true;
}
//---------------------------------------------------------------------

// AABB of a geometry stored in a quadtree node. Can be smaller than node AABB for rightmost and bottommost patches of an imperfectly sized CDLOD.
inline bool CCDLODData::GetPatchAABB(UPTR X, UPTR Z, UPTR LOD, rtm::vector4f& BoxCenter, rtm::vector4f& BoxExtent) const
{
	if (!GetNodeAABB(X, Z, LOD, BoxCenter, BoxExtent)) return false;
	ClampNodeToPatchAABB(BoxCenter, BoxExtent);
	return true;
}
//---------------------------------------------------------------------

// See GetPatchAABB comment
inline void CCDLODData::ClampNodeToPatchAABB(rtm::vector4f& BoxCenter, rtm::vector4f& BoxExtent) const
{
	const auto BoxMax = rtm::vector_add(BoxCenter, BoxExtent);
	if (rtm::vector_any_less_than(BoundsMax, BoxMax))
	{
		const auto HalfExcess = rtm::vector_mul(rtm::vector_sub(BoxMax, rtm::vector_min(BoundsMax, BoxMax)), 0.5f);
		BoxCenter = rtm::vector_sub(BoxCenter, HalfExcess);
		BoxExtent = rtm::vector_sub(BoxExtent, HalfExcess);
	}
}
//---------------------------------------------------------------------

}
