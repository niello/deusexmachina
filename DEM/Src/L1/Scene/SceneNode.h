#pragma once
#ifndef __DEM_L1_SCENE_NODE_H__
#define __DEM_L1_SCENE_NODE_H__

#include <Scene/NodeAttribute.h>
#include <Scene/NodeController.h>
#include <Math/TransformSRT.h>
#include <Data/Flags.h>
#include <Data/StringID.h>
#include <Data/Dictionary.h>

// Scene nodes represent hierarchical transform frames and together form a scene graph.
// Each 3D scene consists of one scene graph starting at the root scene node.

// ?Line, subdiv, deformer?
// shadow & its visibility
//!!!NB - completely unscaled nodes/meshes can be rendered through dual quat skinning!
//???apply scaling ONLY to attributes and not to child nodes?

// Collision shape is attr? it receives transform
// DON'T FORGET, now physics & graphics are separated at entity level

namespace Scene
{
class CScene;
typedef Ptr<class CSceneNode> PSceneNode;

class CSceneNode: public Core::CRefCounted
{
protected:

	enum
	{
		Active				= 0x01,	// Node must be processed
		RespectsLOD			= 0x02,	// This node is affected by parent's LODGroup attribute
		LocalMatrixDirty	= 0x04,	// Local transform components were changed, but matrix is not updated
		WorldMatrixDirty	= 0x08,	// Local matrix changed, and world matrix need to be updated
		WorldMatrixChanged	= 0x10,	// World matrix of this node was changed this frame
		WorldMatrixUpdated	= 0x20,	// World matrix of this node was already updated this frame
		LocalTransformValid	= 0x40	// Local transform is actual, not invalidated by world space controller
	};

	typedef CDict<CStrID, PSceneNode> CNodeDict;

	//???store scene ptr here? or always pass as param?

	CStrID					Name;
	CSceneNode*				pParent;
	CNodeDict				Child;
	CScene*					pScene;

	//!!!can write special 4x3 tfm matrix withour 0,0,0,1 column to save memory! is good for SSE?
	Math::CTransform		Tfm;
	matrix44				LocalMatrix;	// For caching only
	matrix44				WorldMatrix;

	Data::CFlags			Flags; // ?UniformScale?, LockTransform
	nArray<PNodeAttribute>	Attrs; //???or list? List seems to be better

	PNodeController			Controller;

	friend class CScene;

public:

	CSceneNode(CScene& Scene, CStrID NodeName);
	~CSceneNode();

	CSceneNode*				CreateChild(CStrID ChildName);
	void					AddChild(CSceneNode& Node);
	void					RemoveChild(CSceneNode& Node);
	void					RemoveChild(DWORD Idx);
	void					RemoveChild(CStrID ChildName);
	void					RemoveFromParent() { if (pParent) pParent->RemoveChild(*this); }

	CSceneNode*				GetParent() const { return pParent; }
	DWORD					GetChildCount() const { return Child.GetCount(); }
	CSceneNode*				GetChild(DWORD Idx) const { return Child.ValueAt(Idx); }
	//???create and non-create () const; versions?
	CSceneNode*				GetChild(CStrID ChildName, bool Create = false);
	CSceneNode*				GetChild(LPCSTR Path, bool Create = false);
	CSceneNode*				FindChildRecursively(CStrID ChildName, bool OnlyInCurrentSkeleton = true); // Handy to find bones, could stop on skeleton terminating nodes

	bool					AddAttr(CNodeAttribute& Attr);
	DWORD					GetAttrCount() const { return Attrs.GetCount(); }
	CNodeAttribute*			GetAttr(DWORD Idx) const { return Attrs[Idx]; }
	void					RemoveAttr(CNodeAttribute& Attr);
	void					RemoveAttr(DWORD Idx);
	template<class T> T*	FindFirstAttr() const;

	bool					SetController(CNodeController* pCtlr);
	CNodeController*		GetController() const { return Controller.GetUnsafe(); }

	void					UpdateLocalSpace(bool UpdateWorldMatrix = true);
	void					UpdateWorldSpace();
	void					UpdateWorldFromLocal();
	void					UpdateLocalFromWorld();

	// Rendering, lighting & debug rendering
	// (Implement only transforms with debug rendering before writing render connections)

	void					RenderDebug();

	// Animator (controller, animation node) management

	CStrID					GetName() const { return Name; }
	CScene*					GetScene() const { return pScene; }

	bool					IsActive() const { return Flags.Is(Active); }
	void					Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
	bool					IsLODDependent() const { return Flags.Is(RespectsLOD); }
	bool					IsLocalTransformValid() const { return Flags.Is(LocalTransformValid); }
	bool					IsLocalMatrixDirty() const { return Flags.Is(LocalMatrixDirty); }
	bool					IsWorldMatrixDirty() const { return Flags.Is(WorldMatrixDirty); }
	bool					IsWorldMatrixChanged() const { return Flags.Is(WorldMatrixChanged); }

	void					SetPosition(const vector3& Pos) { Tfm.Translation = Pos; Flags.Set(LocalMatrixDirty | LocalTransformValid); }
	const vector3&			GetPosition() const { return Tfm.Translation; }
	void					SetRotation(const quaternion& Rot) { Tfm.Rotation = Rot; Flags.Set(LocalMatrixDirty | LocalTransformValid); }
	const quaternion&		GetRotation() const { return Tfm.Rotation; }
	void					SetScale(const vector3& Scale) { Tfm.Scale = Scale; Flags.Set(LocalMatrixDirty | LocalTransformValid); }
	const vector3&			GetScale() const { return Tfm.Scale; }
	void					SetLocalTransform(const Math::CTransform& NewTfm) { Tfm = NewTfm; Flags.Set(LocalMatrixDirty | LocalTransformValid); }
	void					SetLocalTransform(const matrix44& Transform);
	const Math::CTransform&	GetLocalTransform() const { return Tfm; }
	void					SetWorldTransform(const matrix44& Transform);
	const matrix44&			GetLocalMatrix() const { return LocalMatrix; }
	const matrix44&			GetWorldMatrix() const { return WorldMatrix; }
	const vector3&			GetWorldPosition() const { return WorldMatrix.Translation(); }
};

inline CSceneNode::CSceneNode(CScene& Scene, CStrID NodeName):
	pScene(&Scene),
	pParent(NULL),
	Name(NodeName),
	Flags(Active | LocalMatrixDirty | LocalTransformValid)
{
	Attrs.Flags.Clear(Array_KeepOrder);
}
//---------------------------------------------------------------------

inline CSceneNode::~CSceneNode()
{
	Child.Clear();
	while (Attrs.GetCount()) RemoveAttr(Attrs.GetCount() - 1);
}
//---------------------------------------------------------------------

inline void CSceneNode::RemoveChild(CSceneNode& Node)
{
	n_assert(Node.pParent == this && Child.Erase(Node.Name));
	Node.pParent = NULL;
}
//---------------------------------------------------------------------

inline CSceneNode* CSceneNode::GetChild(CStrID ChildName, bool Create)
{
	int Idx = Child.FindIndex(ChildName);
	if (Idx == INVALID_INDEX) return Create ? CreateChild(ChildName) : NULL;
	return GetChild(Idx);
}
//---------------------------------------------------------------------

template<class T> inline T* CSceneNode::FindFirstAttr() const
{
	for (int i = 0; i < Attrs.GetCount(); ++i)
	{
		CNodeAttribute* pAttr = Attrs[i];
		if (pAttr->IsA(T::RTTI)) return (T*)pAttr;
	}
	return NULL;
}
//---------------------------------------------------------------------

inline void CSceneNode::SetLocalTransform(const matrix44& Transform)
{
	LocalMatrix = Transform;
	Tfm.FromMatrix(LocalMatrix);
	Flags.Clear(LocalMatrixDirty);
	Flags.Set(WorldMatrixDirty | LocalTransformValid);
}
//---------------------------------------------------------------------

}

#endif
