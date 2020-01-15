#include "HeightfieldShape.h"
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace Physics
{

CHeightfieldShape::CHeightfieldShape(btHeightfieldTerrainShape* pShape, PHeightfieldData&& HeightfieldData, const vector3& Offset)
	: CCollisionShape(pShape)
	, _HeightfieldData(std::move(HeightfieldData))
{
}
//---------------------------------------------------------------------

}
