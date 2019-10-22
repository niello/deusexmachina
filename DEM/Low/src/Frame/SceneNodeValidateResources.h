#pragma once
#include <Scene/NodeVisitor.h>
#include <Data/Ptr.h>

// Scene traversal that validates resources of render objects in a scene subtree

namespace Frame
{
class CGraphicsResourceManager;

class CSceneNodeValidateResources: public Scene::INodeVisitor
{
private:

	CGraphicsResourceManager& _ResMgr;

public:

	CSceneNodeValidateResources(CGraphicsResourceManager& ResMgr) : _ResMgr(ResMgr) {}

	virtual bool Visit(Scene::CSceneNode& Node);
};

}
