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

public:

	CHeightfieldShape(btHeightfieldTerrainShape* pShape, PHeightfieldData&& HeightfieldData, const vector3& Offset = vector3::Zero);
};

typedef Ptr<CHeightfieldShape> PHeightfieldShape;

}
