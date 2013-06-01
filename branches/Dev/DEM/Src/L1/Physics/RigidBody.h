#pragma once
#ifndef __DEM_L1_RIGID_BODY_H__
#define __DEM_L1_RIGID_BODY_H__

#include <Physics/CollisionObj.h>

// Rigid body is a center of mass that has shape and transform. Rigid body is
// simulated by physics world and can be used as transformation source.

namespace Scene
{
	class CSceneNode;
}

namespace Physics
{

class CRigidBody: public CCollisionObj
{
	__DeclareClassNoFactory;

public:

	virtual ~CRigidBody() { Term(); }

	//!!!need normal flags!
	virtual bool	Init(const Data::CParams& Desc, const vector3& Offset = vector3::Zero);
	virtual void	Term();
	virtual bool	AttachToLevel(CPhysicsWorld& World);
	virtual void	RemoveFromLevel();

	//!!!get tfm stored in motion state
	//void			GetMotionStateAABB(bbox3& OutBox) const;
};

typedef Ptr<CRigidBody> PRigidBody;

}

#endif
