#pragma once
#include <Scene/NodeVisitor.h>

// Scene traversal that validates physics-related attributes of scene nodes
// TODO: to one application-level visitor that validates all types of resources?

namespace Resources
{
	class CResourceManager;
}

namespace Physics
{
class CPhysicsLevel;

class CSceneNodeValidatePhysics: public Scene::INodeVisitor
{
protected:

	CPhysicsLevel& _Level;

public:

	CSceneNodeValidatePhysics(CPhysicsLevel& Level) : _Level(Level) {}

	virtual bool Visit(Scene::CSceneNode& Node) override;
};

}
