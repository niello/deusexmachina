#include "RigidBodySet.h"
#include <Physics/PhysicsLevel.h>

namespace Physics
{

CRigidBodySet::CRigidBodySet(Scene::CSceneNode& Node /*, rigid body (or its params)*/)
{
	// Single rigid body variation

	// Args: CPhysicsLevel& Level, CCollisionShape& Shape, U16 CollisionGroup, U16 CollisionMask, float Mass, const matrix44& InitialTfm
	// TODO: physics material

	/*
	auto pBtShape = Shape->GetBulletShape();

	Mass = BodyMass;
	btVector3 Inertia;
	pBtShape->calculateLocalInertia(Mass, Inertia);

	CMotionStateDynamic* pMS = new CMotionStateDynamic;
	btRigidBody::btRigidBodyConstructionInfo CI(Mass, pMS, pBtShape, Inertia);

	//!!!set friction and restitution!

	pBtCollObj = new btRigidBody(CI);
	pBtCollObj->setUserPointer(this);

	pWorld->GetBtWorld()->addRigidBody(pRB, Group, Mask);

	// Sometimes we read tfm from motion state before any real tick is performed.
	// For this case make that tfm up-to-date.
	pRB->setInterpolationWorldTransform(pRB->getWorldTransform());
	*/
}
//---------------------------------------------------------------------

CRigidBodySet::~CRigidBodySet()
{
	/*
	pWorld->GetBtWorld()->removeRigidBody(_pBtObject);
	delete _pBtObject->getMotionState();
	*/
}
//---------------------------------------------------------------------

}