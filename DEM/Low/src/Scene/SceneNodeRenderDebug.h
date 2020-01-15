#pragma once
#include <Scene/NodeVisitor.h>

// Renders debug graphics for scene nodes and attributes

namespace Debug
{
	class CDebugDraw;
}

namespace Scene
{

class CSceneNodeRenderDebug: public INodeVisitor
{
protected:

	Debug::CDebugDraw& _DebugDraw;

public:

	CSceneNodeRenderDebug(Debug::CDebugDraw& DebugDraw) : _DebugDraw(DebugDraw) {}

	virtual bool Visit(Scene::CSceneNode& Node);
};

}
