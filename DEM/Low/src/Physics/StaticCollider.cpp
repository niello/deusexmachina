#include "StaticCollider.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/CollisionShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{

CStaticCollider::CStaticCollider(CCollisionShape& Shape, CStrID CollisionGroupID, CStrID CollisionMaskID, const matrix44& InitialTfm, const CPhysicsMaterial& Material)
	: CPhysicsObject(CollisionGroupID, CollisionMaskID)
{
	auto pCollObj = new btCollisionObject();
	pCollObj->setCollisionShape(Shape.GetBulletShape());
	ConstructInternal(pCollObj, Material);

	SetTransform(InitialTfm);
}
//---------------------------------------------------------------------

CStaticCollider::~CStaticCollider()
{
	if (_Level) _Level->GetBtWorld()->removeCollisionObject(_pBtObject);
}
//---------------------------------------------------------------------

void CStaticCollider::AttachToLevelInternal()
{
	const U16 Group = _Level->CollisionGroups.GetMask(_CollisionGroupID ? _CollisionGroupID.CStr() : "PhysicalStatic");
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
	btTransform BtTfm;
	if (PrepareTransform(Tfm, BtTfm))
	{
		_pBtObject->setWorldTransform(BtTfm);
		//???need? if (_Level) _Level->GetBtWorld()->updateSingleAabb(_pBtObject);
	}
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
