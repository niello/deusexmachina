#include "HeightfieldShape.h"
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CHeightfieldShape, Physics::CCollisionShape);

CHeightfieldShape::CHeightfieldShape(btHeightfieldTerrainShape* pShape, PHeightfieldData&& HeightfieldData, const vector3& Offset)
	: CCollisionShape(pShape, Offset)
	, _HeightfieldData(std::move(HeightfieldData))
{
}
//---------------------------------------------------------------------

}
