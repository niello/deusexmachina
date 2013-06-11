#include "PhysicsObj.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsWorld.h>
#include <Data/Params.h>
#include <mathlib/bbox.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CPhysicsObj, Core::CRefCounted);

PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, const nString& FileName);

bool CPhysicsObj::Init(const Data::CParams& Desc, const vector3& Offset)
{
	n_assert(!pWorld);

	CStrID ShapeID = Desc.Get<CStrID>(CStrID("Shape"));
	Shape = PhysicsSrv->CollisionShapeMgr.GetTypedResource(ShapeID);
	if (!Shape.IsValid())
		Shape = LoadCollisionShapeFromPRM(ShapeID, nString("physics:") + ShapeID.CStr() + ".hrd"); //!!!prm!
	n_assert(Shape->IsLoaded());

	Group = PhysicsSrv->CollisionGroups.GetMask(Desc.Get<nString>(CStrID("Group"), "Default"));
	Mask = PhysicsSrv->CollisionGroups.GetMask(Desc.Get<nString>(CStrID("Mask"), "All"));

	ShapeOffset = Offset;

	//???get rid of self-offset? prop can calc heightmap offset
	vector3 ShapeSelfOffset;
	if (Shape->GetOffset(ShapeSelfOffset)) ShapeOffset += ShapeSelfOffset;

	OK;
}
//---------------------------------------------------------------------

bool CPhysicsObj::Init(CCollisionShape& CollShape, ushort CollGroup, ushort CollMask, const vector3& Offset)
{
	n_assert(CollShape.IsLoaded());
	Shape = &CollShape;
	Group = CollGroup;
	Mask = CollMask;

	ShapeOffset = Offset;

	//???get rid of self-offset? prop can calc heightmap offset
	vector3 ShapeSelfOffset;
	if (Shape->GetOffset(ShapeSelfOffset)) ShapeOffset += ShapeSelfOffset;

	OK;
}
//---------------------------------------------------------------------

// Is required to aviod virtual call from destructor
void CPhysicsObj::InternalTerm()
{
	n_assert2(!pWorld, "CPhysicsObj::InternalTerm() -> Object must be removed from level");

	if (pBtCollObj)
	{
		delete pBtCollObj;
		pBtCollObj = NULL;
	}

	Shape = NULL;
}
//---------------------------------------------------------------------

void CPhysicsObj::Term()
{
	RemoveFromLevel();
	InternalTerm();
}
//---------------------------------------------------------------------

bool CPhysicsObj::AttachToLevel(CPhysicsWorld& World)
{
	if (!pBtCollObj) FAIL;

	n_assert(!pWorld);
	pWorld = &World;
	pWorld->AddCollisionObject(*this);

	OK;
}
//---------------------------------------------------------------------

void CPhysicsObj::RemoveFromLevel()
{
	if (pWorld)
	{
		pWorld->RemoveCollisionObject(*this);
		pWorld = NULL;
	}
}
//---------------------------------------------------------------------

void CPhysicsObj::SetTransform(const matrix44& Tfm)
{
	n_assert_dbg(pBtCollObj);

	btTransform BtTfm = TfmToBtTfm(Tfm);

	BtTfm.getOrigin().m_floats[0] += ShapeOffset.x;
	BtTfm.getOrigin().m_floats[1] += ShapeOffset.y;
	BtTfm.getOrigin().m_floats[2] += ShapeOffset.z;

	pBtCollObj->setWorldTransform(BtTfm);
	if (pWorld) pWorld->GetBtWorld()->updateSingleAabb(pBtCollObj);
}
//---------------------------------------------------------------------

void CPhysicsObj::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);

	Out = pBtCollObj->getWorldTransform();

	Out.getOrigin().m_floats[0] -= ShapeOffset.x;
	Out.getOrigin().m_floats[1] -= ShapeOffset.y;
	Out.getOrigin().m_floats[2] -= ShapeOffset.z;
}
//---------------------------------------------------------------------

void CPhysicsObj::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
	btTransform Tfm;
	GetTransform(Tfm);
	OutRot = BtQuatToQuat(Tfm.getRotation());
	OutPos = BtVectorToVector(Tfm.getOrigin());
}
//---------------------------------------------------------------------

// If possible, returns interpolated AABB from motion state. It matches the graphics representation.
void CPhysicsObj::GetGlobalAABB(bbox3& OutBox) const
{
	btTransform Tfm;
	GetTransform(Tfm);

	btVector3 Min, Max;
	pBtCollObj->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox.vmin = BtVectorToVector(Min);
	OutBox.vmax = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

// Returns AABB from the physics world
void CPhysicsObj::GetPhysicsAABB(bbox3& OutBox) const
{
	n_assert_dbg(pBtCollObj);

	btVector3 Min, Max;
	if (pWorld) pWorld->GetBtWorld()->getBroadphase()->getAabb(pBtCollObj->getBroadphaseHandle(), Min, Max);
	else pBtCollObj->getCollisionShape()->getAabb(pBtCollObj->getWorldTransform(), Min, Max);
	OutBox.vmin = BtVectorToVector(Min);
	OutBox.vmax = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

}
