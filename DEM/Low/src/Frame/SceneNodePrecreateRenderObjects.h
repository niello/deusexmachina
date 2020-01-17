#pragma once
#include <Scene/NodeVisitor.h>

// Precreates frame view structures for render-related attributes, loads GPU resources.
// This is done automatically for all new objects at their first draw, but preloading
// can help to avoid freezing. Run this visitor under the loading screen.
// Inactive attributes also load their resources.
// Run CSceneNodeValidateResources before running this visitor.

namespace Frame
{
class CView;

class CSceneNodePrecreateRenderObjects: public Scene::INodeVisitor
{
protected:

	Frame::CView& _View;

public:

	CSceneNodePrecreateRenderObjects(Frame::CView& View) : _View(View) {}

	virtual bool Visit(Scene::CSceneNode& Node) override;
};

}
