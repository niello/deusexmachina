#pragma once
#ifndef __DEM_L1_HEIGHTFIELD_SHAPE_H__
#define __DEM_L1_HEIGHTFIELD_SHAPE_H__

#include <Physics/CollisionShape.h>

// Heightfield collision shape, which owns heightmap data (and possibly can modify it)

class btHeightfieldTerrainShape;

namespace Physics
{

class CHeightfieldShape: public CCollisionShape
{
	__DeclareClass(CHeightfieldShape);

protected:

	void*				pHFData;
	float				HOffset;

public:

	CHeightfieldShape(CStrID ID): CCollisionShape(ID), pHFData(NULL) {}
	virtual ~CHeightfieldShape() { if (IsLoaded()) Unload(); }

	bool			Setup(btHeightfieldTerrainShape* pShape, void* pHeightMapData, float HeightOffset);
	virtual void	Unload();

	float			GetHeightOffset() const { return HOffset; }
};

typedef Ptr<CHeightfieldShape> PHeightfieldShape;

}

#endif
