#pragma once
#ifndef __DEM_L1_SCENE_NODE_H__
#define __DEM_L1_SCENE_NODE_H__

#include <Resources/ResourceObject.h>
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

class CSceneNode: public Resources::CResourceObject
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
	PNodeController				Controller;
	CDict<CStrID, PSceneNode>	Children;	//???or sorted array? don't store IDs twice if possible
	CArray<PNodeAttribute>		Attrs; //???or list? List seems to be better //???WHY?

	void					OnDetachFromScene();

public:

	CSceneNode(CStrID NodeName = CStrID::Empty);
	virtual ~CSceneNode();

	virtual bool			IsResourceValid() const { OK; }

	void					UpdateTransform(const vector3* pCOIArray, UPTR COICount, bool ProcessDefferedController, CArray<CSceneNode*>* pOutDefferedNodes = nullptr);
	void					UpdateWorldFromLocal();
	void					UpdateLocalFromWorld();

	PSceneNode				Clone(bool CloneChildren);
	void					Remove() { if (pParent) pParent->RemoveChild(*this); }

	bool					AcceptVisitor(INodeVisitor& Visitor);

	CStrID					GetName() const { return Name; }

	CSceneNode*				CreateChild(CStrID ChildName);
	CSceneNode*				CreateChildChain(const char* pPath);
	void					AddChild(CStrID ChildName, CSceneNode& Node);
	void					RemoveChild(CSceneNode& Node);
	void					RemoveChild(UPTR Idx);
	void					RemoveChild(CStrID ChildName);
	CSceneNode*				GetParent() const { return pParent; }
	UPTR					GetChildCount() const { return Children.GetCount(); }
	CSceneNode*				GetChild(UPTR Idx) const { return Children.ValueAt(Idx); }
	CSceneNode*				GetChild(CStrID ChildName) const;
	CSceneNode*				GetChild(const char* pPath) const;
	CSceneNode*				FindDeepestChild(const char* pPath, char const* & pUnresolvedPathPart) const;

	bool					SetController(CNodeController* pCtlr);
	CNodeController*		GetController() const { return Controller.Get(); }

	bool					AddAttribute(CNodeAttribute& Attr);
	void					RemoveAttribute(CNodeAttribute& Attr);
	void					RemoveAttribute(UPTR Idx);
	UPTR					GetAttributeCount() const { return Attrs.GetCount(); }
	CNodeAttribute*			GetAttribute(IPTR Idx) const { return Attrs[Idx]; }
	template<class T> T*	FindFirstAttribute() const;

	bool					IsRoot() const { return !pParent; }
	bool					IsChild(const CSceneNode* pParentNode) const;
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

inline void CSceneNode::RemoveChild(CSceneNode& Node)
{
	n_assert(Node.pParent == this);
	Node.OnDetachFromScene();
	Children.Remove(Node.Name);
	Node.pParent = nullptr;
}
//---------------------------------------------------------------------

inline CSceneNode* CSceneNode::GetChild(CStrID ChildName) const
{
	IPTR Idx = Children.FindIndex(ChildName);
	return Idx == INVALID_INDEX ? nullptr : Children.ValueAt(Idx).Get();
}
//---------------------------------------------------------------------

inline CSceneNode* CSceneNode::GetChild(const char* pPath) const
{
	const char* pUnprocessed;
	CSceneNode* pNode = FindDeepestChild(pPath, pUnprocessed);
	return pUnprocessed ? nullptr : pNode;
}
//---------------------------------------------------------------------

template<class T> inline T* CSceneNode::FindFirstAttribute() const
{
	for (UPTR i = 0; i < Attrs.GetCount(); ++i)
	{
		CNodeAttribute* pAttr = Attrs[i];
		if (pAttr->IsA(T::RTTI)) return (T*)pAttr;
	}
	return nullptr;
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
	for (UPTR i = 0; i < Children.GetCount(); ++i)
		if (!Children.ValueAt(i)->AcceptVisitor(Visitor)) FAIL;
	OK;
}
//---------------------------------------------------------------------

inline bool CSceneNode::IsChild(const CSceneNode* pParentNode) const
{
	const CSceneNode* pCurr = pParent;
	while (pCurr)
	{
		if (pCurr == pParentNode) OK;
		pCurr = pCurr->pParent;
	}
	FAIL;
}
//---------------------------------------------------------------------

}

#endif
