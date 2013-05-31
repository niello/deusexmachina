#include "CollisionObjStatic.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsWorld.h>
#include <mathlib/bbox.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObjStatic, Core::CRefCounted);

bool CCollisionObjStatic::Init(CCollisionShape& CollShape, ushort Group, ushort Mask, const vector3& Offset)
{
	n_assert(!pWorld && CollShape.IsLoaded());

	pBtCollObj = new btCollisionObject();
	pBtCollObj->setCollisionShape(CollShape.GetBtShape());
	//???pass material and set friction and restitution?

	return CCollisionObj::Init(CollShape, Group, Mask, Offset);
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
	if (!pWorld) return;
	n_assert(pWorld->GetBtWorld());
	pWorld->GetBtWorld()->removeCollisionObject(pBtCollObj);
	CCollisionObj::RemoveFromLevel();
}
//---------------------------------------------------------------------

}
