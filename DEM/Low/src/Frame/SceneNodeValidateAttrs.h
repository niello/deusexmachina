#pragma once
#include <Scene/NodeVisitor.h>

// Scene traversal that validates attributes of scene nodes. All lazy loading is done there.

namespace Frame
{
class CGraphicsResourceManager;

class CSceneNodeValidateAttrs: public Scene::INodeVisitor
{
protected:

	Frame::CGraphicsResourceManager& _ResMgr;

public:

	CSceneNodeValidateAttrs(Frame::CGraphicsResourceManager& ResMgr);

	virtual bool Visit(Scene::CSceneNode& Node) override;
};

}
