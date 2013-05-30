#pragma once
#ifndef __DEM_L1_COLLISION_OBJECT_MOVING_H__
#define __DEM_L1_COLLISION_OBJECT_MOVING_H__

#include <Physics/CollisionObj.h>

// Moving collision object inherits transform from the scene node, collides with
// dynamic bodies as a moving object, but doesn't respond to collisions.
// Use this type of objects to represent objects controlled by an animation,
// by user input or any other non-physics controller.

namespace Scene
{
	class CSceneNode;
}

namespace Physics
{

class CCollisionObjMoving: public CCollisionObj
{
	__DeclareClassNoFactory;

protected:

	Scene::CSceneNode*	pNode;

public:

	CCollisionObjMoving(): pNode(NULL) {}
	virtual ~CCollisionObjMoving() { Term(); }

	//!!!need normal flags!
	virtual bool	Init(CCollisionShape& CollShape, ushort Group = 0x0001, ushort Mask = 0xffff, const vector3& Offset = vector3::Zero);
	virtual void	Term();
	virtual bool	AttachToLevel(CPhysicsWorld& World);
	virtual void	RemoveFromLevel();

	void			SetNode(Scene::CSceneNode& Node);
	void			GetMotionStateAABB(bbox3& OutBox) const;
};

typedef Ptr<CCollisionObjMoving> PCollisionObjMoving;

}

#endif
