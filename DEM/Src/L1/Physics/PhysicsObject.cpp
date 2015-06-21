#include "PhysicsObject.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsWorld.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Data/Params.h>
#include <Math/AABB.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CPhysicsObject, Core::CObject);

PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, const CString& FileName);

bool CPhysicsObject::Init(const Data::CParams& Desc, const vector3& Offset)
{
	n_assert(!pWorld);

	CStrID ShapeID = Desc.Get<CStrID>(CStrID("Shape"));
	Resources::PResource RShape = ResourceMgr->RegisterResource(ShapeID);
	if (!RShape->IsLoaded())
	{
		Resources::PResourceLoader Loader = ResourceMgr->CreateDefaultLoaderFor<Physics::CCollisionShape>("prm"); //!!!get ext from URI!
		ResourceMgr->LoadResource(RShape, Loader);
		n_assert(RShape->IsLoaded());
	}
	Shape = RShape->GetObject()->As<Physics::CCollisionShape>();

	Group = PhysicsSrv->CollisionGroups.GetMask(Desc.Get<CString>(CStrID("Group"), "Default"));
	Mask = PhysicsSrv->CollisionGroups.GetMask(Desc.Get<CString>(CStrID("Mask"), "All"));

	ShapeOffset = Offset;

	//???get rid of self-offset? prop can calc heightmap offset
	vector3 ShapeSelfOffset;
	if (Shape->GetOffset(ShapeSelfOffset)) ShapeOffset += ShapeSelfOffset;

	OK;
}
//---------------------------------------------------------------------

bool CPhysicsObject::Init(CCollisionShape& CollShape, ushort CollGroup, ushort CollMask, const vector3& Offset)
{
	n_assert(CollShape.IsResourceValid());
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
void CPhysicsObject::InternalTerm()
{
	n_assert2(!pWorld, "CPhysicsObject::InternalTerm() -> Object must be removed from level");

	if (pBtCollObj)
	{
		delete pBtCollObj;
		pBtCollObj = NULL;
	}

	Shape = NULL;
}
//---------------------------------------------------------------------

void CPhysicsObject::Term()
{
	RemoveFromLevel();
	InternalTerm();
}
//---------------------------------------------------------------------

bool CPhysicsObject::AttachToLevel(CPhysicsWorld& World)
{
	if (!pBtCollObj) FAIL;

	n_assert(!pWorld);
	pWorld = &World;
	pWorld->AddCollisionObject(*this);

	OK;
}
//---------------------------------------------------------------------

void CPhysicsObject::RemoveFromLevel()
{
	if (pWorld)
	{
		pWorld->RemoveCollisionObject(*this);
		pWorld = NULL;
	}
}
//---------------------------------------------------------------------

void CPhysicsObject::SetTransform(const matrix44& Tfm)
{
	n_assert_dbg(pBtCollObj);

	btTransform BtTfm = TfmToBtTfm(Tfm);
	BtTfm.getOrigin() = BtTfm * VectorToBtVector(ShapeOffset);

	pBtCollObj->setWorldTransform(BtTfm);
	if (pWorld) pWorld->GetBtWorld()->updateSingleAabb(pBtCollObj);
}
//---------------------------------------------------------------------

void CPhysicsObject::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);
	Out = pBtCollObj->getWorldTransform();
	Out.getOrigin() = Out * VectorToBtVector(-ShapeOffset);
}
//---------------------------------------------------------------------

void CPhysicsObject::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
	btTransform Tfm;
	CPhysicsObject::GetTransform(Tfm); //???to nonvirtual GetWorldTransform()?
	OutRot = BtQuatToQuat(Tfm.getRotation());
	OutPos = BtVectorToVector(Tfm.getOrigin());
}
//---------------------------------------------------------------------

// If possible, returns interpolated AABB from motion state. It matches the graphics representation.
void CPhysicsObject::GetGlobalAABB(CAABB& OutBox) const
{
	btTransform Tfm;
	GetTransform(Tfm);

	btVector3 Min, Max;
	pBtCollObj->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

// Returns AABB from the physics world
void CPhysicsObject::GetPhysicsAABB(CAABB& OutBox) const
{
	n_assert_dbg(pBtCollObj);

	btVector3 Min, Max;
	if (pWorld) pWorld->GetBtWorld()->getBroadphase()->getAabb(pBtCollObj->getBroadphaseHandle(), Min, Max);
	else pBtCollObj->getCollisionShape()->getAabb(pBtCollObj->getWorldTransform(), Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

}
