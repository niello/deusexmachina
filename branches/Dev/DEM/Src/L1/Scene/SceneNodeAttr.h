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
class CScene;
class CSceneNode;

class CSceneNodeAttr: public Core::CRefCounted
{
	DeclareRTTI;

protected:

	CSceneNode* pNode;

public:

	CSceneNodeAttr(): pNode(NULL) {}

	virtual void UpdateTransform(CScene& Scene) = 0;
	virtual void PrepareToRender() {}

	CSceneNode* GetNode() const { return pNode; }
};

typedef Ptr<CSceneNodeAttr> PSceneNodeAttr;

}

#endif
