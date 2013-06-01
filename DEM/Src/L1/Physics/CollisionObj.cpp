#include "CollisionObj.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsWorld.h>
#include <Data/Params.h>
#include <mathlib/bbox.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionObj, Core::CRefCounted);

PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, const nString& FileName);

bool CCollisionObj::Init(const Data::CParams& Desc, const vector3& Offset)
{
	n_assert(!pWorld);

	CStrID ShapeID = Desc.Get<CStrID>(CStrID("Shape"));
	Shape = PhysicsSrv->CollShapeMgr.GetTypedResource(ShapeID);
	if (!Shape.IsValid())
		Shape = LoadCollisionShapeFromPRM(ShapeID, nString("physics:") + ShapeID.CStr() + ".hrd"); //!!!prm!

	//!!!???what if shape is found but is not loaded? RESMGR problem!
	//desired way:
	//if resource is found, but not loaded
	//  pass in into the loader
	//if loader determines that the type is incompatible
	//  it gets the resource pointer from the resource manager
	//  sets it to the passed pointer
	//  checks its type
	//  if type is right, someone reloaded the resource before
	//  else replaces a passed pointer with a new one, of the right type
	n_assert(Shape->IsLoaded());

	Group = (ushort)Desc.Get<int>(CStrID("Group"), 0x0001); //!!!set normal flags!
	Mask = (ushort)Desc.Get<int>(CStrID("Mask"), 0xffff); //!!!set normal flags!
	ShapeOffset = Offset; //???pre-add shape offset? always constant!

	OK;
}
//---------------------------------------------------------------------

void CCollisionObj::Term()
{
	RemoveFromLevel();

	if (pBtCollObj)
	{
		delete pBtCollObj;
		pBtCollObj = NULL;
	}

	Shape = NULL;
}
//---------------------------------------------------------------------

bool CCollisionObj::AttachToLevel(CPhysicsWorld& World)
{
	if (!pBtCollObj) FAIL;

	n_assert(!pWorld);
	pWorld = &World;
	pWorld->AddCollisionObject(*this);

	OK;
}
//---------------------------------------------------------------------

void CCollisionObj::RemoveFromLevel()
{
	if (pWorld)
	{
		pWorld->RemoveCollisionObject(*this);
		pWorld = NULL;
	}
}
//---------------------------------------------------------------------

void CCollisionObj::SetTransform(const matrix44& Tfm)
{
	n_assert_dbg(pBtCollObj);

	btTransform BtTfm = TfmToBtTfm(Tfm);

	BtTfm.getOrigin().m_floats[0] += ShapeOffset.x;
	BtTfm.getOrigin().m_floats[1] += ShapeOffset.y;
	BtTfm.getOrigin().m_floats[2] += ShapeOffset.z;

	vector3 ShapeSelfOffset;
	if (Shape->GetOffset(ShapeSelfOffset))
	{
		BtTfm.getOrigin().m_floats[0] += ShapeSelfOffset.x;
		BtTfm.getOrigin().m_floats[1] += ShapeSelfOffset.y;
		BtTfm.getOrigin().m_floats[2] += ShapeSelfOffset.z;
	}

	pBtCollObj->setWorldTransform(BtTfm);
	if (pWorld) pWorld->GetBtWorld()->updateSingleAabb(pBtCollObj);
}
//---------------------------------------------------------------------

void CCollisionObj::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);

	Out = pBtCollObj->getWorldTransform();

	Out.getOrigin().m_floats[0] -= ShapeOffset.x;
	Out.getOrigin().m_floats[1] -= ShapeOffset.y;
	Out.getOrigin().m_floats[2] -= ShapeOffset.z;

	vector3 ShapeSelfOffset;
	if (Shape->GetOffset(ShapeSelfOffset))
	{
		Out.getOrigin().m_floats[0] -= ShapeSelfOffset.x;
		Out.getOrigin().m_floats[1] -= ShapeSelfOffset.y;
		Out.getOrigin().m_floats[2] -= ShapeSelfOffset.z;
	}
}
//---------------------------------------------------------------------

void CCollisionObj::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
	btTransform Tfm;
	GetTransform(Tfm);
	OutRot = BtQuatToQuat(Tfm.getRotation());
	OutPos = BtVectorToVector(Tfm.getOrigin());
}
//---------------------------------------------------------------------

void CCollisionObj::GetGlobalAABB(bbox3& OutBox) const
{
	btTransform Tfm;
	GetTransform(Tfm);

	btVector3 Min, Max;
	pBtCollObj->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox.vmin = BtVectorToVector(Min);
	OutBox.vmax = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

void CCollisionObj::GetPhysicsAABB(bbox3& OutBox) const
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
