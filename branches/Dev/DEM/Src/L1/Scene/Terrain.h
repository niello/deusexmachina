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
	float				VerticalScale;
	bbox3				Box;

	short*				pMinMaxData;
	nArray<short*>		MinMaxMaps;

	Render::PTexture	HeightMap;

public:

	CTerrain(): MinMaxMaps(2, 1), pMinMaxData(NULL) { }

	virtual bool		LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);
	virtual bool		OnAdd();
	virtual void		OnRemove();
	virtual void		Update();

	DWORD				GetPatchSize() const { return PatchSize; }
	Render::CTexture*	GetHeightMap() const { return HeightMap.get_unsafe(); }
};

RegisterFactory(CTerrain);

typedef Ptr<CTerrain> PTerrain;

}

#endif
