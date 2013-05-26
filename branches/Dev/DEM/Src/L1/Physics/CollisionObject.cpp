#include "CollisionObject.h"

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObject, Core::CRefCounted);

CCollisionObject::CCollisionObject(CCollisionShape& CollShape): Shape(&CollShape)
{
	n_assert(Shape->IsLoaded());
	pBtCollObj = new btCollisionObject();
	pBtCollObj->setCollisionShape(Shape->GetBtShape());
	//???pass material and set friction and restitution?
}
//---------------------------------------------------------------------

//!!!store strong ref in the physworld!
CCollisionObject::~CCollisionObject()
{
	delete pBtCollObj;
}
//---------------------------------------------------------------------

}
