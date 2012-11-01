#pragma once
#ifndef __DEM_L1_SCENE_NODE_ATTR_H__
#define __DEM_L1_SCENE_NODE_ATTR_H__

#include <Core/RefCounted.h>
#include <Data/Flags.h>
#include <Data/StringID.h>

// Scene node attributes implement specific logic, attached to 3D transform provided by scene nodes.
// Common examples are meshes, lights, cameras etc.

namespace Scene
{
class CSceneNode;

class CSceneNodeAttr: public Core::CRefCounted
{
private:

	CSceneNode* pNode;

public:

	virtual void Update() = 0;
	virtual void PrepareToRender() {}
};

typedef Ptr<class CSceneNodeAttr> PSceneNodeAttr;

}

#endif
