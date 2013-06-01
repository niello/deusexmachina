#include "CollisionObjStatic.h"

#include <Physics/PhysicsWorld.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObjStatic, Physics::CCollisionObj);

bool CCollisionObjStatic::Init(const Data::CParams& Desc, const vector3& Offset)
{
	if (!CCollisionObj::Init(Desc, Offset)) FAIL;

	pBtCollObj = new btCollisionObject();
	pBtCollObj->setCollisionShape(Shape->GetBtShape());
	pBtCollObj->setUserPointer(this);

	//!!!set friction and restitution!

	OK;
}
//---------------------------------------------------------------------

bool CCollisionObjStatic::AttachToLevel(CPhysicsWorld& World)
{
	if (!CCollisionObj::AttachToLevel(World)) FAIL;
	pWorld->GetBtWorld()->addCollisionObject(pBtCollObj, Group, Mask);
	OK;
}
//---------------------------------------------------------------------

void CCollisionObjStatic::RemoveFromLevel()
{
	if (!pWorld || !pWorld->GetBtWorld()) return;
	pWorld->GetBtWorld()->removeCollisionObject(pBtCollObj);
	CCollisionObj::RemoveFromLevel();
}
//---------------------------------------------------------------------

}
