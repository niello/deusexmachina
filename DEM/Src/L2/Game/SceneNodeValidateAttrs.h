#pragma once
#ifndef __DEM_L2_SNV_VALIDATE_ATTRS_H__
#define __DEM_L2_SNV_VALIDATE_ATTRS_H__

#include <Scene/NodeVisitor.h>
#include <Data/Ptr.h>

// Scene traversal that validates attributes of scene nodes. All lazy loading is done there.
// Level is provided as current context for resource initialization.

namespace Game
{
typedef Ptr<class CGameLevel> PGameLevel;

class CSceneNodeValidateAttrs: public Scene::INodeVisitor
{
public:

	PGameLevel Level;

	virtual bool Visit(Scene::CSceneNode& Node);
};

}

#endif
