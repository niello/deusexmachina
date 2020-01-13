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
RTTI_CLASS_IMPL(Physics::CRigidBody, Core::CObject); //Physics::CPhysicsObject);

// To be useful, rigid body must be connected to a scene node to serve as its transformation source
class CDynamicMotionState: public btMotionState
{
protected:

	Scene::PSceneNode _Node;

public:

	BT_DECLARE_ALIGNED_ALLOCATOR();

	void SetSceneNode(Scene::PSceneNode Node) { _Node = Node; }

	virtual void getWorldTransform(btTransform& worldTrans) const override
	{
		//!!!update world matrix if dirty inside GetWorldMatrix()!
		const matrix44& Tfm = _Node ? _Node->GetWorldMatrix() : matrix44::Identity;
		worldTrans = TfmToBtTfm(Tfm);
	}

	virtual void setWorldTransform(const btTransform& worldTrans) override
	{
		if (_Node) _Node->SetWorldTransform(BtTfmToTfm(worldTrans));
	}
};

//!!!dynamic motion state shape offset?

CRigidBody::CRigidBody(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, float Mass, const matrix44& InitialTfm)
	: _Level(&Level)
{
	// Instead of storing strong ref, we manually control refcount and use
	// a pointer from the bullet collision shape
	Shape.AddRef();

	btVector3 Inertia;
	Shape.GetBulletShape()->calculateLocalInertia(Mass, Inertia);

	btRigidBody::btRigidBodyConstructionInfo CI(
		Mass,
		new CDynamicMotionState(/*Node or initial tfm*/),
		Shape.GetBulletShape(),
		Inertia);
	//!!!set friction and restitution! for spheres always need rolling friction! TODO: physics material

	_pBtObject = new btRigidBody(CI);
	_pBtObject->setUserPointer(this);

	_Level->GetBtWorld()->addRigidBody(_pBtObject, CollisionGroup, CollisionMask);

	// Sometimes we read tfm from motion state before any real tick is performed.
	// For this case make that tfm up-to-date.
	//???is getWorldTransform up to date?! must set before!
	_pBtObject->setInterpolationWorldTransform(_pBtObject->getWorldTransform());
}
//---------------------------------------------------------------------

CRigidBody::~CRigidBody()
{
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	_Level->GetBtWorld()->removeRigidBody(_pBtObject);
	delete _pBtObject->getMotionState();
	delete _pBtObject;

	// See constructor
	pShape->Release();
}
//---------------------------------------------------------------------

// After node change:
//// Enforce offline transform update to be taken into account
//pRB->setMotionState(pRB->getMotionState());

void CRigidBody::SetTransform(const matrix44& Tfm)
{
	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		btTransform BtTfm = TfmToBtTfm(Tfm);
		BtTfm.getOrigin() = BtTfm * VectorToBtVector(ShapeOffset);
		pMotionState->setWorldTransform(BtTfm);
	}
	if (!pMotionState || pWorld) CPhysicsObject::SetTransform(Tfm);
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
	n_assert_dbg(pBtCollObj);

	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		OutRot = BtQuatToQuat(pMotionState->Rotation);
		OutPos = BtVectorToVector(pMotionState->Position) - OutRot.rotate(ShapeOffset);
	}
	else CPhysicsObject::GetTransform(OutPos, OutRot);
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(btTransform& Out) const
{
	n_assert_dbg(pBtCollObj);

	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState)
	{
		pMotionState->getWorldTransform(Out);
		Out.getOrigin() = Out * VectorToBtVector(-ShapeOffset);
	}
	else CPhysicsObject::GetTransform(Out);
}
//---------------------------------------------------------------------

void CRigidBody::SetTransformChanged(bool Changed)
{
	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	if (pMotionState) pMotionState->TfmChanged = Changed;
}
//---------------------------------------------------------------------

bool CRigidBody::IsTransformChanged() const
{
	CMotionStateDynamic* pMotionState = (CMotionStateDynamic*)((btRigidBody*)pBtCollObj)->getMotionState();
	return pMotionState ? pMotionState->TfmChanged : true;
}
//---------------------------------------------------------------------

float CRigidBody::GetInvMass() const
{
	return ((btRigidBody*)pBtCollObj)->getInvMass();
}
//---------------------------------------------------------------------

bool CRigidBody::IsActive() const
{
	int ActState = ((btRigidBody*)pBtCollObj)->getActivationState();
	return ActState == DISABLE_DEACTIVATION || ActState == ACTIVE_TAG;
}
//---------------------------------------------------------------------

bool CRigidBody::IsAlwaysActive() const
{
	return ((btRigidBody*)pBtCollObj)->getActivationState() == DISABLE_DEACTIVATION;
}
//---------------------------------------------------------------------

bool CRigidBody::IsAlwaysInactive() const
{
	return ((btRigidBody*)pBtCollObj)->getActivationState() == DISABLE_SIMULATION;
}
//---------------------------------------------------------------------

void CRigidBody::MakeActive()
{
	((btRigidBody*)pBtCollObj)->forceActivationState(ACTIVE_TAG);
}
//---------------------------------------------------------------------

void CRigidBody::MakeAlwaysActive()
{
	((btRigidBody*)pBtCollObj)->forceActivationState(DISABLE_DEACTIVATION);
}
//---------------------------------------------------------------------

void CRigidBody::MakeAlwaysInactive()
{
	((btRigidBody*)pBtCollObj)->forceActivationState(DISABLE_SIMULATION);
}
//---------------------------------------------------------------------

}