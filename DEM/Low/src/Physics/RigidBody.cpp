#include "RigidBody.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/CollisionShape.h>
#include <Scene/SceneNode.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{

CRigidBody::CDynamicMotionState::CDynamicMotionState(const rtm::vector4f& Offset) : _Offset(Offset) {}
//---------------------------------------------------------------------

CRigidBody::CDynamicMotionState::~CDynamicMotionState() = default;
//---------------------------------------------------------------------

void CRigidBody::CDynamicMotionState::SetSceneNode(Scene::PSceneNode&& Node)
{
	_Node = std::move(Node);
}
//---------------------------------------------------------------------

void CRigidBody::CDynamicMotionState::getWorldTransform(btTransform& worldTrans) const
{
	worldTrans = Math::ToBullet(_Node ? _Node->GetWorldMatrix() : rtm::matrix_identity());
	worldTrans.getOrigin() = worldTrans * Math::ToBullet3(_Offset);
}
//---------------------------------------------------------------------

void CRigidBody::CDynamicMotionState::setWorldTransform(const btTransform& worldTrans)
{
	if (_Node)
	{
		auto Tfm = Math::FromBullet(worldTrans);
		Tfm.w_axis = rtm::matrix_mul_point3(rtm::vector_neg(_Offset), Tfm);
		_Node->SetWorldTransform(Tfm);
	}
}
//---------------------------------------------------------------------

CRigidBody::CRigidBody(float Mass, CCollisionShape& Shape, CStrID CollisionGroupID, CStrID CollisionMaskID, const rtm::matrix3x4f& InitialTfm, const CPhysicsMaterial& Material)
	: CPhysicsObject(CollisionGroupID, CollisionMaskID)
	, _MotionState(Shape.GetOffset())
{
	n_assert(Mass != 0.f);

	btRigidBody::btRigidBodyConstructionInfo CI(Mass, &_MotionState, Shape.GetBulletShape());
	Shape.GetBulletShape()->calculateLocalInertia(Mass, CI.m_localInertia);
	ConstructInternal(new btRigidBody(CI), Material);

	SetTransform(InitialTfm);
	_pBtObject->setInterpolationWorldTransform(_pBtObject->getWorldTransform());
}
//---------------------------------------------------------------------

CRigidBody::~CRigidBody()
{
	if (_Level) _Level->GetBtWorld()->removeRigidBody(static_cast<btRigidBody*>(_pBtObject));
}
//---------------------------------------------------------------------

void CRigidBody::AttachToLevelInternal()
{
	const auto& Groups = _Level->PredefinedCollisionGroups;
	const auto Group = _CollisionGroupID ? _Level->CollisionGroups.GetMask(_CollisionGroupID.CStr()) : Groups.Dynamic;
	const auto Mask = _CollisionMaskID ? _Level->CollisionGroups.GetMask(_CollisionMaskID.CStr()) : (Groups.Dynamic | Groups.Static | Groups.Query);
	_Level->GetBtWorld()->addRigidBody(static_cast<btRigidBody*>(_pBtObject), Group, Mask);
}
//---------------------------------------------------------------------

void CRigidBody::RemoveFromLevelInternal()
{
	_Level->GetBtWorld()->removeRigidBody(static_cast<btRigidBody*>(_pBtObject));
}
//---------------------------------------------------------------------

void CRigidBody::SetControlledNode(Scene::CSceneNode* pNode)
{
	_MotionState.SetSceneNode(pNode);
	if (pNode)
	{
		if (_Level) _Level->GetBtWorld()->synchronizeSingleMotionState(static_cast<btRigidBody*>(_pBtObject));
		_pBtObject->activate();
	}
}
//---------------------------------------------------------------------

void CRigidBody::SetTransform(const rtm::matrix3x4f& Tfm)
{
	btTransform BtTfm;
	if (PrepareTransform(Tfm, BtTfm))
	{
		_pBtObject->setWorldTransform(BtTfm);
		_pBtObject->activate();
	}
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(rtm::matrix3x4f& OutTfm) const
{
	if (const Scene::CSceneNode* pSceneNode = _MotionState.GetSceneNode())
	{
		OutTfm = pSceneNode->GetWorldMatrix();
	}
	else
	{
		auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
		OutTfm = Math::FromBullet(_pBtObject->getWorldTransform());
		OutTfm.w_axis = rtm::matrix_mul_point3(rtm::vector_neg(pShape->GetOffset()), OutTfm);
	}
}
//---------------------------------------------------------------------

// Interpolated AABB from motion state, matches the graphics representation
void CRigidBody::GetGlobalAABB(Math::CAABB& OutBox) const
{
	btTransform Tfm;
	if (_MotionState.GetSceneNode())
		_MotionState.getWorldTransform(Tfm);
	else
		Tfm = _pBtObject->getWorldTransform(); //???or getInterpolationWorldTransform()?

	btVector3 Min, Max;
	_pBtObject->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox = Math::AABBFromMinMax(Math::FromBullet(Min), Math::FromBullet(Max));
}
//---------------------------------------------------------------------

void CRigidBody::SetActive(bool Active, bool Always)
{
	// Dynamic object activates and deactivates normally
	if (Always)
	{
		_pBtObject->forceActivationState(Active ? DISABLE_DEACTIVATION : DISABLE_SIMULATION);
	}
	else if (Active)
	{
		// Need to force state first because "Always" states arent't automatically reverted to ACTIVE_TAG.
		// Nevertheless, call activate() to reset deactivation timer.
		_pBtObject->forceActivationState(ACTIVE_TAG);
		_pBtObject->activate();
	}
	else
	{
		_pBtObject->forceActivationState(WANTS_DEACTIVATION);
	}
}
//---------------------------------------------------------------------

float CRigidBody::GetInvMass() const
{
	return static_cast<btRigidBody*>(_pBtObject)->getInvMass();
}
//---------------------------------------------------------------------

btRigidBody* CRigidBody::GetBtBody() const
{
	return static_cast<btRigidBody*>(_pBtObject);
}
//---------------------------------------------------------------------

}
