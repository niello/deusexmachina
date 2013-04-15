#pragma once
#ifndef __DEM_L1_SCENE_TERRAIN_H__
#define __DEM_L1_SCENE_TERRAIN_H__

//#include <Scene/SceneNodeAttr.h>
#include <Scene/RenderObject.h>
#include <Render/Materials/Texture.h>
#include <Render/Geometry/Mesh.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling
// and integrated visibility test.

namespace Scene
{

class CTerrain: public CRenderObject //CSceneNodeAttr
{
	DeclareRTTI;
	DeclareFactory(CTerrain);

protected:

	DWORD				HFWidth;
	DWORD				HFHeight;
	DWORD				PatchSize;
	DWORD				LODCount;
	DWORD				TopPatchCountX;
	DWORD				TopPatchCountZ;
	float				VerticalScale;
	bbox3				Box;

	struct CMinMaxMap
	{
		DWORD	PatchesW;
		DWORD	PatchesH;
		short*	pData;
	};

	short*				pMinMaxData;
	nArray<CMinMaxMap>	MinMaxMaps;

	Render::PTexture	HeightMap;

public:

	CTerrain(): MinMaxMaps(2, 1), pMinMaxData(NULL) { }

	virtual bool		LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);
	virtual bool		OnAdd();
	virtual void		OnRemove();
	virtual void		Update();

	DWORD				GetHeightMapWidth() const { return HFWidth; }
	DWORD				GetHeightMapHeight() const { return HFHeight; }
	DWORD				GetPatchSize() const { return PatchSize; }
	DWORD				GetLODCount() const { return LODCount; }
	DWORD				GetTopPatchCountX() const { return TopPatchCountX; }
	DWORD				GetTopPatchCountZ() const { return TopPatchCountZ; }
	float				GetVerticalScale() const { return VerticalScale; }
	const bbox3&		GetLocalAABB() const { return Box; }
	void				GetGlobalAABB(bbox3& Out) const;
	Render::CTexture*	GetHeightMap() const { return HeightMap.get_unsafe(); }
	void				GetMinMaxHeight(DWORD X, DWORD Z, DWORD LOD, short& MinY, short& MaxY) const;
	bool				HasNode(DWORD X, DWORD Z, DWORD LOD) const;
};

RegisterFactory(CTerrain);

typedef Ptr<CTerrain> PTerrain;

inline void CTerrain::GetMinMaxHeight(DWORD X, DWORD Z, DWORD LOD, short& MinY, short& MaxY) const
{
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	const short* pMMRec = MMMap.pData + ((Z * MMMap.PatchesW + X) << 1);
	MinY = *pMMRec;
	MaxY = *(pMMRec + 1);
}
//---------------------------------------------------------------------

inline bool CTerrain::HasNode(DWORD X, DWORD Z, DWORD LOD) const
{
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	return X < MMMap.PatchesW && Z < MMMap.PatchesH;
}
//---------------------------------------------------------------------

}

#endif
