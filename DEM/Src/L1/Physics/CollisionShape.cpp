#include "CollisionShape.h"

#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionShape, Core::CRefCounted);

bool CCollisionShape::Setup(btCollisionShape* pShape)
{
	if (!pShape)
	{
		State = Resources::Rsrc_Failed;
		FAIL;
	}

	pBtShape = pShape;
	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CCollisionShape::Unload()
{
	n_assert(GetRefCount() <= 1); //!!!if unload when used, physics will crash!
	if (pBtShape)
	{
		delete pBtShape;
		pBtShape = NULL;
	}
	State = Resources::Rsrc_NotLoaded;
}
//---------------------------------------------------------------------

}
