#include "HeightfieldShape.h"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace Physics
{
__ImplementResourceClass(Physics::CHeightfieldShape, 'HFSH', Physics::CCollisionShape);

bool CHeightfieldShape::Setup(btHeightfieldTerrainShape* pShape, void* pHeightMapData, float HeightOffset)
{
	if (pHeightMapData && CCollisionShape::Setup(pShape))
	{
		pHFData = pHeightMapData;
		HOffset = HeightOffset;
		OK;
	}

	State = Resources::Rsrc_Failed;
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
