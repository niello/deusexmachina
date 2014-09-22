#pragma once
#ifndef __DEM_L1_SNV_VALIDATE_RESOURCES_H__
#define __DEM_L1_SNV_VALIDATE_RESOURCES_H__

#include <Scene/NodeVisitor.h>

// Scene traversal that validates resources of render objects in a scene subtree

namespace Render
{

class CSceneNodeValidateResources: public Scene::CNodeVisitor
{
public:

	bool Visit(Scene::CSceneNode& Node);
};

}

#endif
