#pragma once
#ifndef __DEM_L1_SCENE_TERRAIN_H__
#define __DEM_L1_SCENE_TERRAIN_H__

#include <Scene/SceneNodeAttr.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling,
// integrated visibility test, and isn't a render object itself

namespace Scene
{

class CTerrain: public CSceneNodeAttr
{
	DeclareRTTI;
	DeclareFactory(CTerrain);

protected:

public:

	CTerrain() { }

	virtual bool	OnAdd();
	virtual bool	LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);
	virtual void	Update();
};

RegisterFactory(CTerrain);

typedef Ptr<CTerrain> PTerrain;

}

#endif
