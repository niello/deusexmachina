#include "StaticCollider.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/HeightfieldShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CStaticCollider, Physics::CPhysicsObject);

CStaticCollider::CStaticCollider(CCollisionShape& Shape, CStrID CollisionGroupID, CStrID CollisionMaskID, const matrix44& InitialTfm)
	: CPhysicsObject(CollisionGroupID, CollisionMaskID)
{
	// Instead of storing strong ref, we manually control refcount and use
	// a pointer from the bullet collision shape
	Shape.AddRef();

	//!!!set friction and restitution! for spheres always need rolling friction! TODO: physics material

	_pBtObject = new btCollisionObject();
	_pBtObject->setCollisionShape(Shape.GetBulletShape());
	_pBtObject->setWorldTransform(TfmToBtTfm(InitialTfm)); //!!!shape offset!
	_pBtObject->setUserPointer(this);

	// As of Bullet v2.81 SDK, debug drawer tries to draw each heightfield triangle wireframe,
	// so we disable debug drawing of terrain at all
	// TODO: terrain is most probably a static collider, not movable!
	if (Shape.IsA<CHeightfieldShape>())
		_pBtObject->setCollisionFlags(_pBtObject->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
}
//---------------------------------------------------------------------

CStaticCollider::~CStaticCollider()
{
	if (_Level) _Level->GetBtWorld()->removeCollisionObject(_pBtObject);

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	delete _pBtObject;
	pShape->Release(); // See constructor
}
//---------------------------------------------------------------------

void CStaticCollider::AttachToLevelInternal()
{
	const U16 Group = _Level->CollisionGroups.GetMask(_CollisionGroupID ? _CollisionGroupID.CStr() : "Default");
	const U16 Mask = _Level->CollisionGroups.GetMask(_CollisionMaskID ? _CollisionMaskID.CStr() : "All");
	_Level->GetBtWorld()->addCollisionObject(_pBtObject, Group, Mask);
}
//---------------------------------------------------------------------

void CStaticCollider::RemoveFromLevelInternal()
{
	_Level->GetBtWorld()->removeCollisionObject(_pBtObject);
}
//---------------------------------------------------------------------

void CStaticCollider::SetTransform(const matrix44& Tfm)
{
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	btTransform BtTfm = TfmToBtTfm(Tfm);
	BtTfm.getOrigin() = BtTfm * VectorToBtVector(pShape->GetOffset());
	_pBtObject->setWorldTransform(BtTfm);
	//???need? if (_Level) _Level->GetBtWorld()->updateSingleAabb(_pBtObject);
}
//---------------------------------------------------------------------

void CStaticCollider::GetTransform(matrix44& OutTfm) const
{
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	OutTfm = BtTfmToTfm(_pBtObject->getWorldTransform());
	OutTfm.Translation() = OutTfm * (-pShape->GetOffset());
}
//---------------------------------------------------------------------

// Interpolated AABB from motion state, matches the graphics representation
void CStaticCollider::GetGlobalAABB(CAABB& OutBox) const
{
	// Static object is not interpolated, use physics AABB
	GetPhysicsAABB(OutBox);
}
//---------------------------------------------------------------------

}
