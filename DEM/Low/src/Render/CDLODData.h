#pragma once
#include <Core/Object.h>
#include <Data/FixedArray.h>
#include <Math/AABB.h>
#include <acl/math/vector4_32.h>
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
	float                   VerticalScale;
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
		float                VerticalScale;
		std::array<float, 4> SplatMapUVCoeffs;
		U32                  MinMaxDataCount;
	};
#pragma pack(pop)

	U32					GetHeightMapWidth() const { return HFWidth; }
	U32					GetHeightMapHeight() const { return HFHeight; }
	U32					GetPatchSize() const { return PatchSize; }
	U32					GetLODCount() const { return LODCount; }
	float               GetVerticalScale() const { return VerticalScale; }
	const CAABB&		GetAABB() const { return Box; }
	void				GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, I16& MinY, I16& MaxY) const;
	void				GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, float& MinY, float& MaxY) const;
	bool				HasNode(UPTR X, UPTR Z, UPTR LOD) const;
	bool				GetNodeAABB(UPTR X, UPTR Z, UPTR LOD, acl::Vector4_32& BoxCenter, acl::Vector4_32& BoxExtent) const;
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
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	return X < MMMap.PatchesW && Z < MMMap.PatchesH;
}
//---------------------------------------------------------------------

inline bool CCDLODData::GetNodeAABB(UPTR X, UPTR Z, UPTR LOD, acl::Vector4_32& BoxCenter, acl::Vector4_32& BoxExtent) const
{
	I16 MinY, MaxY;
	GetMinMaxHeight(X, Z, LOD, MinY, MaxY);

	// Node has no data
	if (MaxY < MinY) return false;

	const float NodeHalfSize = static_cast<float>(PatchSize << LOD) * 0.5f;
	const float MinYF = MinY * VerticalScale;
	const float MaxYF = MaxY * VerticalScale;

	BoxCenter = acl::vector_set((2 * X + 1) * NodeHalfSize, (MaxYF + MinYF) * 0.5f, (2 * Z + 1) * NodeHalfSize);
	BoxExtent = acl::vector_set(NodeHalfSize, (MaxYF - MinYF) * 0.5f, NodeHalfSize);

	return true;
}
//---------------------------------------------------------------------

}
