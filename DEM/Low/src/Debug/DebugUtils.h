#pragma once
#include <Scene/SceneNode.h>

// Various debug utility functions

namespace DEM::Debug
{

void PrintSceneNodeHierarchy(const Scene::CSceneNode& Node, size_t Indent = 0);

}
