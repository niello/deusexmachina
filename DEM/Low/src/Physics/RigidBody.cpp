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
	Scene::CSceneNode* GetSceneNode() const { return _Node.Get(); }

	//!!!dynamic motion state shape offset?
	virtual void getWorldTransform(btTransform& worldTrans) const override
	{
		//!!!update world matrix if dirty inside GetWorldMatrix()!
		worldTrans = TfmToBtTfm(_Node ? _Node->GetWorldMatrix() : matrix44::Identity);
	}

	//!!!dynamic motion state shape offset?
	virtual void setWorldTransform(const btTransform& worldTrans) override
	{
		if (_Node) _Node->SetWorldTransform(BtTfmToTfm(worldTrans));
	}
};

CRigidBody::CRigidBody(CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, float Mass, const matrix44& InitialTfm)
	: _Level(&Level)
{
	n_assert(Mass != 0.f);

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

void CRigidBody::SetTransform(const matrix44& Tfm)
{
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	btTransform BtTfm = TfmToBtTfm(Tfm);
	BtTfm.getOrigin() = BtTfm * VectorToBtVector(pShape->GetOffset());
	_pBtObject->setWorldTransform(BtTfm);
	_pBtObject->activate();
}
//---------------------------------------------------------------------

void CRigidBody::GetTransform(vector3& OutPos, quaternion& OutRot) const
{
	auto pMotionState = static_cast<CDynamicMotionState*>(_pBtObject->getMotionState());
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	btTransform Tfm;
	if (pMotionState->GetSceneNode())
		_pBtObject->getMotionState()->getWorldTransform(Tfm); //!!!can return Node's non-converted world tfm right here!
	else
		Tfm = _pBtObject->getWorldTransform();

	OutRot = BtQuatToQuat(Tfm.getRotation());
	OutPos = BtVectorToVector(Tfm.getOrigin()) - OutRot.rotate(pShape->GetOffset());
}
//---------------------------------------------------------------------

float CRigidBody::GetInvMass() const
{
	return _pBtObject->getInvMass();
}
//---------------------------------------------------------------------

void CRigidBody::SetActive(bool Active, bool Always)
{
	if (Always)
		_pBtObject->forceActivationState(Active ? DISABLE_DEACTIVATION : DISABLE_SIMULATION);
	else if (Active)
		_pBtObject->activate();
	else
		_pBtObject->forceActivationState(WANTS_DEACTIVATION);
}
//---------------------------------------------------------------------

bool CRigidBody::IsActive() const
{
	return _pBtObject->isActive();
}
//---------------------------------------------------------------------

}