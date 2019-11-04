#pragma once
#include <Scene/NodeVisitor.h>

// Scene traversal that validates GPU resources of render objects in a scene subtree

namespace Frame
{
class CGraphicsResourceManager;

class CSceneNodeValidateGPUResources: public Scene::INodeVisitor
{
private:

	CGraphicsResourceManager& _ResMgr;

public:

	CSceneNodeValidateGPUResources(CGraphicsResourceManager& ResMgr) : _ResMgr(ResMgr) {}

	virtual bool Visit(Scene::CSceneNode& Node);
};

}
