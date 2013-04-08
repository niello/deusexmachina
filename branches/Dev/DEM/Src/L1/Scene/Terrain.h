#pragma once
#ifndef __DEM_L1_SCENE_TERRAIN_H__
#define __DEM_L1_SCENE_TERRAIN_H__

#include <Scene/SceneNodeAttr.h>
#include <Render/Materials/Texture.h>
#include <Render/Geometry/Mesh.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling,
// integrated visibility test, and isn't a render object itself

namespace Scene
{

class CTerrain: public CSceneNodeAttr
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

	CTerrain(): pMinMaxData(NULL) { }

	virtual bool	LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);
	virtual bool	OnAdd();
	virtual void	OnRemove();
	virtual void	Update();
};

RegisterFactory(CTerrain);

typedef Ptr<CTerrain> PTerrain;

}

#endif
