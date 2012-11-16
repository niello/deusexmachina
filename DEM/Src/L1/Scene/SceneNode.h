#pragma once
#ifndef __DEM_L1_SCENE_NODE_H__
#define __DEM_L1_SCENE_NODE_H__

#include <Scene/SceneNodeAttr.h>
#include <Math/TransformSRT.h>
#include <Data/Flags.h>
#include <Data/StringID.h>
#include <util/ndictionary.h>

// Scene nodes represent hierarchical transform frames and together form a scene graph.
// Each 3D scene consists of one scene graph starting at the root scene node.

// Skinning, LOD

//???apply scaling ONLY to attributes and not to child nodes?

// Mesh, lighting, camera, material, lod, bones
// controllers
// ?Line, subdiv, deformer?
// shadow & its visibility
// Ctlrs: animation with blending, physics (from rigid body), ai and targeting, ?input?
//???allow blending between any controllers? Blend code scope.
///matrix vs quaternion blending. what is dual quaternion?
//what is quat squad?
// Rendering from camera to RT in general, before the final scene is processed, separate visibility checks

// Rigid body could be a controller, it drives transform
// Collision shape is attr? it receives transform
// DON'T FORGET, now physics & graphics are separated at entity level

namespace Scene
{
class CScene;
typedef Ptr<class CSceneNode> PSceneNode;

class CSceneNode: public Core::CRefCounted //???derive from transform source? for noew PropTfm always contain SceneNode
{
public:

	enum
	{
		Active			= 0x01,	// Node must be processed
		OwnedByScene	= 0x02,	// This node is owned by scene and lives until scene is destroyed
		RespectsLOD		= 0x04	// This node is affected by parent's LODGroup attribute
	};

private:

	typedef nDictionary<CStrID, CSceneNode*> CNodeDict;

	//???store scene ptr here? or always pass as param?

	CStrID					Name;
	PSceneNode				Parent;
	CNodeDict				Child;

	Math::CTransform		Tfm;
	matrix44				GlobalTfm;

	Data::CFlags			Flags; // IsRoot, IsDirty, IsLocalTfm, ?UniformScale?, LockTransform, TfmChangedLastFrame
	nArray<PSceneNodeAttr>	Attrs; //???or list? List seems to be better
	// Controller(s)

	friend class CScene;
	friend class CSceneServer;

public:

	~CSceneNode() { if (Parent.isvalid()) Parent->RemoveChild(*this); }

	PSceneNode	CreateChild(CStrID ChildName);
	void		AddChild(CSceneNode& Node);
	void		RemoveChild(CSceneNode& Node);
	void		RemoveChild(DWORD Idx);
	void		RemoveChild(CStrID ChildName);

	CSceneNode*	GetParent() const { return Parent; }
	DWORD		GetChildCount() const { return Child.Size(); }
	CSceneNode*	GetChild(DWORD Idx) const { return Child.ValueAtIndex(Idx); }
	PSceneNode	GetChild(CStrID ChildName, bool Create = false);
	PSceneNode	GetChild(LPCSTR Path, bool Create = false);
	CSceneNode*	FindChildRecursively(CStrID ChildName, bool OnlyInCurrentSkeleton = true); // Handy to find bones, could stop on skeleton terminating nodes

	void		UpdateTransform(CScene& Scene);

	// Rendering, lighting & debug rendering
	// (Implement only transforms with debug rendering before writing render connections)

	void		RenderDebug();

	// Attribute managenent

	// Animator (controller, animation node) management

	void					SetLocalTransform(const matrix44& Transform) { Tfm.FromMatrix(Transform); }
	const Math::CTransform&	GetLocalTransform() { return Tfm; }
	const matrix44&			GetWorldTransform() { return GlobalTfm; }

	CStrID		GetName() const { return Name; }

	bool		IsActive() const { return Flags.Is(Active); }
	void		Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
	bool		IsOwnedByScene() const { return Flags.Is(OwnedByScene); }
	bool		IsLODDependent() const { return Flags.Is(RespectsLOD); }
};

inline void CSceneNode::RemoveChild(CSceneNode& Node)
{
	n_assert(Node.Parent.get_unsafe() == this && Child.Erase(Node.Name));
	Node.Parent = NULL;
}
//---------------------------------------------------------------------

inline PSceneNode CSceneNode::GetChild(CStrID ChildName, bool Create)
{
	int Idx = Child.FindIndex(ChildName);
	if (Idx == INVALID_INDEX) return Create ? CreateChild(ChildName) : NULL;
	return GetChild(Idx);
}
//---------------------------------------------------------------------

}

#endif
