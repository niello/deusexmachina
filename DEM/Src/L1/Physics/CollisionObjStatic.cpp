#include "CollisionObjStatic.h"

#include <Physics/PhysicsWorld.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObjStatic, Physics::CPhysicsObj);

bool CCollisionObjStatic::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CPhysicsObj::Init(Desc, Offset)) FAIL;
	return InternalInit();
}
//---------------------------------------------------------------------

bool CCollisionObjStatic::Init(CCollisionShape& CollShape, ushort CollGroup, ushort CollMask, const vector3& Offset)
{
	if (!CPhysicsObj::Init(CollShape, CollGroup, CollMask, Offset)) FAIL;
	return InternalInit();
}
//---------------------------------------------------------------------

bool CCollisionObjStatic::InternalInit()
{
	pBtCollObj = new btCollisionObject();
	pBtCollObj->setCollisionShape(Shape->GetBtShape());
	pBtCollObj->setUserPointer(this);

	//!!!set friction and restitution!

	OK;
}
//---------------------------------------------------------------------

bool CCollisionObjStatic::AttachToLevel(CPhysicsWorld& World)
{
	if (!CPhysicsObj::AttachToLevel(World)) FAIL;
	pWorld->GetBtWorld()->addCollisionObject(pBtCollObj, Group, Mask);
	OK;
}
//---------------------------------------------------------------------

void CCollisionObjStatic::RemoveFromLevel()
{
	if (!pWorld || !pWorld->GetBtWorld()) return;
	pWorld->GetBtWorld()->removeCollisionObject(pBtCollObj);
	CPhysicsObj::RemoveFromLevel();
}
//---------------------------------------------------------------------

}
