#include "CollisionShape.h"
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CCollisionShape, Resources::CResourceObject);

CCollisionShape::CCollisionShape(btCollisionShape* pShape)
	: pBtShape(pShape)
{
	n_assert(pBtShape && !pBtShape->getUserPointer());
	pBtShape->setUserPointer(this);
}
//---------------------------------------------------------------------

CCollisionShape::~CCollisionShape()
{
	delete pBtShape;
}
//---------------------------------------------------------------------

}
