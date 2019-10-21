#pragma once
#include <Scene/NodeVisitor.h>
#include <Data/Ptr.h>

// Scene traversal that validates resources of render objects in a scene subtree

namespace Frame
{
class CFrameResourceManager;

class CSceneNodeValidateResources: public Scene::INodeVisitor
{
private:

	CFrameResourceManager& _ResMgr;

public:

	CSceneNodeValidateResources(CFrameResourceManager& ResMgr) : _ResMgr(ResMgr) {}

	virtual bool Visit(Scene::CSceneNode& Node);
};

}
