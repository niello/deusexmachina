#pragma once
#include <Physics/CollisionShape.h>
#include <Math/Vector3.h>

// Heightfield collision shape, which owns heightmap data (and possibly can modify it)

class btHeightfieldTerrainShape;

namespace Physics
{
typedef std::unique_ptr<char[]> PHeightfieldData;

class CHeightfieldShape : public CCollisionShape
{
protected:

	PHeightfieldData _HeightfieldData;

	// Bullet shape is created with an origin at the center of a heightmap AABB.
	// This is an offset between that center and the real origin.
	vector3          _Offset;

public:

	CHeightfieldShape(btHeightfieldTerrainShape* pShape, PHeightfieldData&& HeightfieldData, const vector3& Offset);

	virtual const vector3& GetOffset() const override { return _Offset; } // Could use btCompoundShape instead
};

typedef Ptr<CHeightfieldShape> PHeightfieldShape;

}
