#pragma once
#ifndef __DEM_L1_MOTION_STATE_KINEMATIC_H__
#define __DEM_L1_MOTION_STATE_KINEMATIC_H__

#include <LinearMath/btMotionState.h>
#include <Physics/BulletConv.h>
#include <Scene/SceneNode.h>

namespace Physics
{

class CMotionStateFromNode: public btMotionState
{
protected:

	Scene::CSceneNode* pNode;

public:

	CMotionStateFromNode(Scene::CSceneNode& SceneNode): pNode(&SceneNode) {}
	// virtual ~CMotionStateFromNode() { }

	virtual void				getWorldTransform(btTransform& worldTrans) const {}
	virtual void				setWorldTransform(const btTransform& worldTrans) {}

	void						SetNode(Scene::CSceneNode& SceneNode) { pNode = &SceneNode; }
	const Scene::CSceneNode*	GetNode() const { return pNode; }
};

}

#endif
