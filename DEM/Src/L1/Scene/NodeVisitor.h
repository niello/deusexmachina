#pragma once
#ifndef __DEM_L1_SCENE_NODE_VISITOR_H__
#define __DEM_L1_SCENE_NODE_VISITOR_H__

// Base class to utilize a Visitor design pattern on the scene graph hierarchy.
// Implement different scene traversal actions in subclasses.

namespace Scene
{

class CNodeVisitor
{
public:

	bool Visit(class CSceneNode& Node);
};

}

#endif
