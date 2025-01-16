#include "StaticCollider.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/CollisionShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{

CStaticCollider::CStaticCollider(CCollisionShape& Shape, CStrID CollisionGroupID, CStrID CollisionMaskID, const rtm::matrix3x4f& InitialTfm, const CPhysicsMaterial& Material)
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
	const auto& Groups = _Level->PredefinedCollisionGroups;
	const auto Group = _CollisionGroupID ? _Level->CollisionGroups.GetMask(_CollisionGroupID.CStr()) : Groups.Static;
	const auto Mask = _CollisionMaskID ? _Level->CollisionGroups.GetMask(_CollisionMaskID.CStr()) : (Groups.Dynamic | Groups.Query);
	_Level->GetBtWorld()->addCollisionObject(_pBtObject, Group, Mask);
}
//---------------------------------------------------------------------

void CStaticCollider::RemoveFromLevelInternal()
{
	_Level->GetBtWorld()->removeCollisionObject(_pBtObject);
}
//---------------------------------------------------------------------

void CStaticCollider::SetTransform(const rtm::matrix3x4f& Tfm)
{
	btTransform BtTfm;
	if (PrepareTransform(Tfm, BtTfm))
	{
		_pBtObject->setWorldTransform(BtTfm);
		//???need? if (_Level) _Level->GetBtWorld()->updateSingleAabb(_pBtObject);
	}
}
//---------------------------------------------------------------------

void CStaticCollider::GetTransform(rtm::matrix3x4f& OutTfm) const
{
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	OutTfm = Math::FromBullet(_pBtObject->getWorldTransform());
	OutTfm.w_axis = rtm::matrix_mul_point3(rtm::vector_neg(pShape->GetOffset()), OutTfm);
}
//---------------------------------------------------------------------

// Interpolated AABB from motion state, matches the graphics representation
void CStaticCollider::GetGlobalAABB(Math::CAABB& OutBox) const
{
	// Static object is not interpolated, use physics AABB
	GetPhysicsAABB(OutBox);
}
//---------------------------------------------------------------------

}
