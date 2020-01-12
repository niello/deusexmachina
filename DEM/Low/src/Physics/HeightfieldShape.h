#pragma once
#include <Physics/CollisionShape.h>
#include <Math/Vector3.h>

// Heightfield collision shape, which owns heightmap data (and possibly can modify it)

class btHeightfieldTerrainShape;

namespace Physics
{

class CHeightfieldShape: public CCollisionShape
{
	FACTORY_CLASS_DECL;

protected:

	void*   pHFData = nullptr;

	// Bullet shape is created with an origin at the center of a heightmap AABB
	// This is an offset between that center and the real origin
	vector3 Offset;

public:

	virtual ~CHeightfieldShape() override { Unload(); }

	bool			Setup(btHeightfieldTerrainShape* pShape, void* pHeightMapData, const vector3& ShapeOffset);
	virtual void	Unload();
	virtual bool	GetOffset(vector3& Out) const { Out = Offset; OK; }
};

typedef Ptr<CHeightfieldShape> PHeightfieldShape;

}
