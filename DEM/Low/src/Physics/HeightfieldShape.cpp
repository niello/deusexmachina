#include "HeightfieldShape.h"

#include <Core/Factory.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace Physics
{
__ImplementClass(Physics::CHeightfieldShape, 'HFSH', Physics::CCollisionShape);

bool CHeightfieldShape::Setup(btHeightfieldTerrainShape* pShape, void* pHeightMapData, const vector3& ShapeOffset)
{
	if (pHeightMapData && CCollisionShape::Setup(pShape))
	{
		pHFData = pHeightMapData;
		Offset = ShapeOffset;
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

void CHeightfieldShape::Unload()
{
	n_assert(GetRefCount() <= 1); //!!!if unload when used, physics will crash!
	SAFE_FREE(pHFData);
	CCollisionShape::Unload();
}
//---------------------------------------------------------------------

}
