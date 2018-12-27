#pragma once
#ifndef __DEM_L1_SCENE_NODE_VISITOR_H__
#define __DEM_L1_SCENE_NODE_VISITOR_H__

// Interface to a Visitor, that allows to use this design pattern with the scene graph hierarchy.
// Implement different scene traversal actions in subclasses.

namespace Scene
{

class INodeVisitor
{
public:

	virtual bool Visit(class CSceneNode& Node) = 0;
};

}

#endif
