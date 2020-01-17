#pragma once
#include <Scene/NodeVisitor.h>

// Validates CPU (RAM) resources of attributes. Call it to preload resources after the scene
// is loaded or cloned from template. Note that if you run this visitor on a scene template,
// resources will remain loaded until the template itself is unloaded.
// Inactive attributes also load their resources.

namespace Resources
{
	class CResourceManager;
}

namespace Scene
{

class CSceneNodeValidateResources: public Scene::INodeVisitor
{
protected:

	Resources::CResourceManager& _ResMgr;

public:

	CSceneNodeValidateResources(Resources::CResourceManager& ResMgr) : _ResMgr(ResMgr) {}

	virtual bool Visit(Scene::CSceneNode& Node) override;
};

}
