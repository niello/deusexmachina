#pragma once
#ifndef __DEM_L1_SCENE_NODE_ATTR_COLLISION_H__
#define __DEM_L1_SCENE_NODE_ATTR_COLLISION_H__

#include <Scene/NodeAttribute.h>
#include <Physics/CollisionObjMoving.h>

// Scene node attribute, that adds a link from scene node transformation
// to a movable collision object in a physics world. Use this attribute to
// add a collision shape to an object in a scene.
// NB: for now this attribute is not loadable from .scn, because at the L1
// there is no any link between certain scene and physics world.

//???!!!can also add CNodeAttrJoint! to connect rigid bodies to non-physical scene nodes

namespace Physics
{

class CNodeAttrCollision: public Scene::CNodeAttribute
{
	__DeclareClassNoFactory;

public:

	PCollisionObjMoving	CollObj;

	virtual Scene::PNodeAttribute	Clone();
	virtual void					Update(const vector3* pCOIArray, UPTR COICount);
	void							GetGlobalAABB(CAABB& OutBox) const { CollObj->GetGlobalAABB(OutBox); }
};

typedef Ptr<CNodeAttrCollision> PNodeAttrCollision;

}

#endif
