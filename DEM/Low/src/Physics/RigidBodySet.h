#pragma once
#include <Physics/PhysicsObject.h>

// A set of rigid bodies and constraints between them controls a set
// of scene nodes very similar to an animation player. Writes world transforms.
// Can be used for single rigid bodies, ragdolls and other compounds.

// TODO: create from .bullet, create ragdoll

class btRigidBody;
class btTypedConstraint;

namespace Scene
{
	class CSceneNode;
}

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
typedef std::unique_ptr<class CRigidBodySet> PRigidBodySet;

class CRigidBodySet final
{
protected:

	PPhysicsLevel                   _Level;
	std::vector<btRigidBody*>       _BtObjects;
	std::vector<btTypedConstraint*> _BtConstraints;

public:

	CRigidBodySet(Scene::CSceneNode& Node /*, rigid body (or its params)*/);
	~CRigidBodySet();

	// Add body
	// Add constraint
};

}
