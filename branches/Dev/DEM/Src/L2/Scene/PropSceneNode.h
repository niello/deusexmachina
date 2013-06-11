#pragma once
#ifndef __DEM_L2_PROP_SCENE_NODE_H__
#define __DEM_L2_PROP_SCENE_NODE_H__

#include <Game/Property.h>
#include <Scene/SceneNode.h>
#include <mathlib/bbox.h>

// Scene node property associates entity transform with scene graph node

namespace Prop
{

class CPropSceneNode: public Game::CProperty
{
	__DeclareClass(CPropSceneNode);
	__DeclarePropertyStorage;

protected:

	Scene::PSceneNode	Node;
	bool				ExistingNode;

	virtual bool		InternalActivate();
	virtual void		InternalDeactivate();

	DECLARE_EVENT_HANDLER(SetTransform, OnSetTransform);
	DECLARE_EVENT_HANDLER(OnWorldTfmsUpdated, OnWorldTfmsUpdated);
	DECLARE_EVENT_HANDLER_VIRTUAL(OnRenderDebug, OnRenderDebug);

	virtual void SetTransform(const matrix44& NewTfm);

public:

	//virtual ~CPropSceneNode() {}

	Scene::CSceneNode*	GetNode() const { return Node.GetUnsafe(); }
	void				GetAABB(bbox3& OutBox) const;
};

}

#endif
