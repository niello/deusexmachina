#include "CollisionShape.h"
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CCollisionShape, Resources::CResourceObject);

CCollisionShape::CCollisionShape(btCollisionShape* pShape)
	: pBtShape(pShape)
{
	// NB: it is very important to store resource object pointer inside a bullet shape.
	// This pointer is used in manual refcounting in CMovableCollider and similar shape users.
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
