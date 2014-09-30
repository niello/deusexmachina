#pragma once
#ifndef __DEM_L1_SCENE_NODE_H__
#define __DEM_L1_SCENE_NODE_H__

#include <Scene/NodeAttribute.h> // definition is required by FindFirstAttr()
#include <Scene/NodeVisitor.h>
#include <Data/Dictionary.h>
#include <Math/TransformSRT.h>

// Scene nodes represent hierarchical transform frames and together form a scene graph.
// Each 3D scene consists of one scene graph starting at the root scene node.
// Each scene node can contain:
// - child nodes, which inherit its transformation
// - controller, which changes its transformation
// - attributes, which receive its transformation

//!!!NB - completely unscaled nodes/meshes can be rendered through dual quat skinning!
//???apply scaling ONLY to attributes and not to child nodes? what about skeletons in such a situation?
// DON'T FORGET, now physics & graphics are separated at entity level

namespace Scene
{
typedef Ptr<class CSceneNode> PSceneNode;
typedef Ptr<class CNodeController> PNodeController;

class CSceneNode: public Core::CObject
{
protected:

	enum
	{
		Active				= 0x01,	// Node must be processed
		LocalTransformValid	= 0x02,	// Local transform is actual and not invalidated by world space controller
		LocalMatrixDirty	= 0x04,	// Local transform components were changed, but matrix is not updated
		WorldMatrixDirty	= 0x08,	// Local matrix changed, and world matrix need to be updated
		WorldMatrixChanged	= 0x10	// World matrix of this node was changed this frame
	};

	CStrID						Name;
	Data::CFlags				Flags; // ?UniformScale?, LockTransform (now lock tfm through static controller)

	//!!!can write special 4x3 tfm matrix without 0,0,0,1 column to save memory! is good for SSE?
	Math::CTransformSRT			Tfm;
	matrix44					LocalMatrix;	// For caching only
	matrix44					WorldMatrix;

	CSceneNode*					pParent;
	CDict<CStrID, PSceneNode>	Children;
	PNodeController				Controller;
	CArray<PNodeAttribute>		Attrs; //???or list? List seems to be better //???WHY?

public:

	CSceneNode(CStrID NodeName);
	virtual ~CSceneNode();

	void					UpdateTransform(const vector3* pCOIArray, DWORD COICount, bool ProcessDefferedController, CArray<CSceneNode*>* pOutDefferedNodes = NULL);
	void					UpdateWorldFromLocal();
	void					UpdateLocalFromWorld();

	void					Remove() { if (pParent) pParent->RemoveChild(*this); }

	bool					AcceptVisitor(INodeVisitor& Visitor);

	CStrID					GetName() const { return Name; }

	CSceneNode*				CreateChild(CStrID ChildName);
	void					AddChild(CSceneNode& Node);
	void					RemoveChild(CSceneNode& Node);
	void					RemoveChild(DWORD Idx);
	void					RemoveChild(CStrID ChildName);
	CSceneNode*				GetParent() const { return pParent; }
	DWORD					GetChildCount() const { return Children.GetCount(); }
	CSceneNode*				GetChild(DWORD Idx) const { return Children.ValueAt(Idx); }
	//???create and non-create () const; versions?
	CSceneNode*				GetChild(CStrID ChildName, bool Create = false);
	CSceneNode*				GetChild(LPCSTR pPath, bool Create = false);

	bool					SetController(CNodeController* pCtlr);
	CNodeController*		GetController() const { return Controller.GetUnsafe(); }

	bool					AddAttribute(CNodeAttribute& Attr);
	void					RemoveAttribute(CNodeAttribute& Attr);
	void					RemoveAttribute(DWORD Idx);
	DWORD					GetAttributeCount() const { return Attrs.GetCount(); }
	CNodeAttribute*			GetAttribute(DWORD Idx) const { return Attrs[Idx]; }
	template<class T> T*	FindFirstAttribute() const;

	bool					IsRoot() const { return !pParent; }
	bool					IsActive() const { return Flags.Is(Active); }
	void					Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
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

inline CSceneNode::CSceneNode(CStrID NodeName):
	pParent(NULL),
	Name(NodeName),
	Flags(Active | LocalMatrixDirty | LocalTransformValid)
{
	Attrs.Flags.Clear(Array_KeepOrder);
}
//---------------------------------------------------------------------

inline void CSceneNode::RemoveChild(CSceneNode& Node)
{
	n_assert(Node.pParent == this && Children.Remove(Node.Name));
	Node.pParent = NULL;
}
//---------------------------------------------------------------------

inline CSceneNode* CSceneNode::GetChild(CStrID ChildName, bool Create)
{
	int Idx = Children.FindIndex(ChildName);
	if (Idx == INVALID_INDEX) return Create ? CreateChild(ChildName) : NULL;
	return GetChild(Idx);
}
//---------------------------------------------------------------------

template<class T> inline T* CSceneNode::FindFirstAttribute() const
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

inline void CSceneNode::SetWorldTransform(const matrix44& Transform)
{
	WorldMatrix = Transform;
	Flags.Set(WorldMatrixChanged);
	UpdateLocalFromWorld();
}
//---------------------------------------------------------------------

inline bool CSceneNode::AcceptVisitor(INodeVisitor& Visitor)
{
	if (!Visitor.Visit(*this)) FAIL;
	for (int i = 0; i < Children.GetCount(); ++i)
		if (!Children.ValueAt(i)->AcceptVisitor(Visitor)) FAIL;
	OK;
}
//---------------------------------------------------------------------

}

#endif
