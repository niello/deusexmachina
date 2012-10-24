#pragma once
#ifndef __DEM_L1_SCENE_NODE_H__
#define __DEM_L1_SCENE_NODE_H__

#include <Data/Flags.h>

// Scene nodes represent hierarchical transform frames and together form a scene graph.
// Each 3D scene consists of one scene graph starting at the root scene node.

namespace Scene
{
typedef Ptr<class CSceneNode> PSceneNode;

class CSceneNode: public Core::CRefCounted
{
private:

	CSceneNode*				pParent;
	nArray<PSceneNode>		Child; //???or list?

	//!!!Transform info Math::CTransform3D
	vector3					LocalPosition;
	quaternion				LocalRotation;
	vector3					LocalScale;

	CFlags					Flags; // IsRoot, IsDirty, ?UniformScale?
	//nArray<PSceneNodeAttr>	Attrs; //???or list?

public:

};

}

#endif
