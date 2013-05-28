#include "CollisionObject.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsWorld.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObject, Core::CRefCounted);

CCollisionObject::CCollisionObject(CCollisionShape& CollShape): Shape(&CollShape), pWorld(NULL)
{
	n_assert(Shape->IsLoaded());
	pBtCollObj = new btCollisionObject();
	pBtCollObj->setCollisionShape(Shape->GetBtShape());
	//???pass material and set friction and restitution?

	//pBtCollObj->setUserPointer(this);
}
//---------------------------------------------------------------------

//!!!store strong ref in the physworld!
CCollisionObject::~CCollisionObject()
{
	delete pBtCollObj;
}
//---------------------------------------------------------------------

bool CCollisionObject::SetTransform(const matrix44& Tfm)
{
	pBtCollObj->setWorldTransform(TfmToBtTfm(Tfm));
	//???only if pBtCollObj is a btCollisionObject? now always!
	if (pWorld) pWorld->GetBtWorld()->updateSingleAabb(pBtCollObj);
	OK;
}
//---------------------------------------------------------------------

}
