#pragma once
#ifndef __DEM_L2_PROP_SCENE_NODE_H__
#define __DEM_L2_PROP_SCENE_NODE_H__

#include <Physics/Prop/PropTransformable.h> //!!!not in physics!
#include <Scene/SceneNode.h>

// Scene node property associates entity transform with scene graph node

//!!!
// Derived from Transformable
// if no scene node at path, create and set transform
// if there is existing node (check OwnedByScene), read transform from it (!!!can avoid storing in DB!)
// Must update entitie's transform from scene node transform, if it changes

namespace Attr
{
	DeclareString(ScenePath);
	DeclareString(SceneFile);
}

namespace Properties
{

class CPropSceneNode: public Game::CProperty //CPropTransformable
{
	DeclareRTTI;
	DeclareFactory(CPropSceneNode);

	//!!!remove if : public CPropTransformable
	DeclarePropertyStorage;
	DeclarePropertyPools(Game::LivePool);

protected:

	Scene::PSceneNode	Node;
	bool				ExistingNode;

public:

	//virtual ~CPropSceneNode() {}

	virtual void		GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void		Activate();
	virtual void		Deactivate();

	Scene::CSceneNode*	GetNode() const { return Node.get_unsafe(); }
};

RegisterFactory(CPropSceneNode);

}

#endif
