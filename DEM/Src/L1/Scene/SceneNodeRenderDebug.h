#pragma once
#ifndef __DEM_L1_SNV_RENDER_DEBUG_H__
#define __DEM_L1_SNV_RENDER_DEBUG_H__

#include <Scene/NodeVisitor.h>

// Scene traversal that renders a debug graphics for a scene graph part

namespace Scene
{

class CSceneNodeRenderDebug: public INodeVisitor
{
public:

	virtual bool Visit(Scene::CSceneNode& Node);
};

}

#endif
