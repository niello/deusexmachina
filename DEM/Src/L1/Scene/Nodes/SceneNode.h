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

	CStrID					Name;
	CSceneNode*				pParent;
	nArray<PSceneNode>		Child; //???or list?

	//!!!Transform info Math::CTransform3D
	// Could be local or global? Or if global tfm is set by controllers, it is set to another var(s)?
	vector3					Position;
	quaternion				Rotation;
	vector3					Scale;
	//matrix44 GlobalTfm;

	CFlags					Flags; // IsRoot, IsDirty, IsLocalTfm, ?UniformScale?, LockTransform, TfmChangedLastFrame
	//nArray<PSceneNodeAttr>	Attrs; //???or list? list is better, cause often only 1 attr is specified
	// Controller(s)

public:

	CSceneNode(): pParent(NULL) {}

	void		AddChild(CSceneNode* pChild);
	void		RemoveChild(DWORD Idx);
	CSceneNode*	FindChild(LPCSTR pName); //, bool Recursive = true); //???!!!long names?!
	DWORD		GetChildCount() const { return Child.Size(); }
	CSceneNode*	GetChild(DWORD Idx) const { return Child[Idx].get_unsafe(); }
	CSceneNode*	GetParent() const { return pParent; }

	// Transform evaluation
	void		UpdateTransform();

	// Rendering, lighting & debug rendering
	// (Implement only transforms with debug rendering before writing render connections)

	// Attribute managenent

	// Animator (controller, animation node) management
};

}

#endif
