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

CRigidBody::CDynamicMotionState::CDynamicMotionState(const vector3& Offset) : _Offset(Offset) {}
//---------------------------------------------------------------------

CRigidBody::CDynamicMotionState::~CDynamicMotionState() = default;
//---------------------------------------------------------------------

void CRigidBody::CDynamicMotionState::getWorldTransform(btTransform& worldTrans) const
{
	worldTrans = TfmToBtTfm(_Node ? _Node->GetWorldMatrix() : matrix44::Identity);
	worldTrans.getOrigin() = worldTrans * VectorToBtVector(_Offset);
}
//---------------------------------------------------------------------

void CRigidBody::CDynamicMotionState::setWorldTransform(const btTransform& worldTrans)
{
	if (_Node)
	{
		auto Tfm = BtTfmToTfm(worldTrans);
		Tfm.Translation() = Tfm * (-_Offset);
		_Node->SetWorldTransform(Tfm);
	}
}
//---------------------------------------------------------------------

CRigidBody::CRigidBody(float Mass, CCollisionShape& Shape, CStrID CollisionGroupID, CStrID CollisionMaskID, const matrix44& InitialTfm, const CPhysicsMaterial& Material)
	: CPhysicsObject(CollisionGroupID, CollisionMaskID)
	, _MotionState(Shape.GetOffset())
{
	n_assert(Mass != 0.f);

	// Instead of storing strong ref, we manually control refcount and use
	// a pointer from the bullet collision shape
	Shape.AddRef();

	btVector3 Inertia;
	Shape.GetBulletShape()->calculateLocalInertia(Mass, Inertia);

	btRigidBody::btRigidBodyConstructionInfo CI(Mass, &_MotionState, Shape.GetBulletShape(), Inertia);
	SetupInternalObject(new btRigidBody(CI), Shape, Material);
	_pBtObject->setWorldTransform(TfmToBtTfm(InitialTfm)); //???shape offset?
	_pBtObject->setInterpolationWorldTransform(_pBtObject->getWorldTransform());
}
//---------------------------------------------------------------------

CRigidBody::~CRigidBody()
{
	if (_Level) _Level->GetBtWorld()->removeRigidBody(static_cast<btRigidBody*>(_pBtObject));

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	delete _pBtObject;
	pShape->Release(); // See constructor
}
//---------------------------------------------------------------------

void CRigidBody::AttachToLevelInternal()
{
	const U16 Group = _Level->CollisionGroups.GetMask(_CollisionGroupID ? _CollisionGroupID.CStr() : "Default");
	const U16 Mask = _Level->CollisionGroups.GetMask(_CollisionMaskID ? _CollisionMaskID.CStr() : "All");
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

void CRigidBody::SetTransform(const matrix44& Tfm)
{
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	btTransform BtTfm = TfmToBtTfm(Tfm);
	BtTfm.getOrigin() = BtTfm * VectorToBtVector(pShape->GetOffset());
	_pBtObject->setWorldTransform(BtTfm);
	_pBtObject->activate();
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(matrix44& OutTfm) const
{
	if (_MotionState.GetSceneNode())
	{
		OutTfm = _MotionState.GetSceneNode()->GetWorldMatrix();
	}
	else
	{
		auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
		OutTfm = BtTfmToTfm(_pBtObject->getWorldTransform());
		OutTfm.Translation() = OutTfm * (-pShape->GetOffset());
	}
}
//---------------------------------------------------------------------

// Interpolated AABB from motion state, matches the graphics representation
void CRigidBody::GetGlobalAABB(CAABB& OutBox) const
{
	btTransform Tfm;
	if (_MotionState.GetSceneNode())
		_MotionState.getWorldTransform(Tfm);
	else
		Tfm = _pBtObject->getWorldTransform(); //???or getInterpolationWorldTransform()?

	btVector3 Min, Max;
	_pBtObject->getCollisionShape()->getAabb(Tfm, Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
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
