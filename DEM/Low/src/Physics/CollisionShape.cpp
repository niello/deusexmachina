#include "CollisionShape.h"

#include <Core/Factory.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
FACTORY_CLASS_IMPL(Physics::CCollisionShape, 'CSHP', Resources::CResourceObject);

bool CCollisionShape::Setup(btCollisionShape* pShape)
{
	if (!pShape) FAIL;
	n_assert(!pShape->getUserPointer());
	pShape->setUserPointer(this);
	pBtShape = pShape;
	OK;
}
//---------------------------------------------------------------------

void CCollisionShape::Unload()
{
	n_assert(GetRefCount() <= 1); //!!!if unload when used, physics will crash!
	if (pBtShape)
	{
		delete pBtShape;
		pBtShape = nullptr;
	}
}
//---------------------------------------------------------------------

}
