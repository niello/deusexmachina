#pragma once
#include <Resources/ResourceObject.h>
#include <Data/StringID.h>
#include <Data/Flags.h>
#include <Math/TransformSRT.h>

// Scene nodes represent hierarchical transform frames and together form a scene graph.
// Each 3D scene consists of one scene graph starting at the root scene node.
// Each scene node can contain:
// - child nodes, which inherit its transformation
// - attributes, which receive its transformation
// Controllers, which change node transformation, are completely external.

// NB: an attempt was made to implement on-demand update of node transformations.
// This leads to repeated recursive parent checking each time tfm is accessed.
// Reverted back to a single hierarchy update traversal.

//!!!completely unscaled nodes/meshes can be rendered through dual quat skinning!

namespace Scene
{
typedef Ptr<class CSceneNode> PSceneNode;
typedef Ptr<class CNodeAttribute> PNodeAttribute;
class INodeVisitor;

class CSceneNode: public Resources::CResourceObject
{
protected:

	enum
	{
		SelfActive          = 0x01, // Node itself is active
		EffectivelyActive   = 0x02, // Node is effectively active, meaning it has no inactive parents
		LocalTransformDirty = 0x04,
		WorldTransformDirty = 0x08
		// TODO: add LockTransform? (now locked through static pose)
	};

	CStrID						Name;
	Data::CFlags				Flags;
	U32                         TransformVersion = 1;
	U32                         LastParentTransformVersion = 0;

	//!!!can write special 4x3 tfm matrix without 0,0,0,1 column to save memory! is good for SSE?
	Math::CTransform			LocalTfm;
	matrix44					WorldMatrix;

	CSceneNode*					pParent = nullptr;
	std::vector<PSceneNode>     Children; // Always sorted by ID
	std::vector<PNodeAttribute> Attrs;

	void					UpdateInternal(const vector3* pCOIArray, UPTR COICount);
	void					UpdateWorldTransform();
	void					UpdateLocalTransform();

	void                    SetParent(CSceneNode* pNewParent);
	void                    UpdateActivity();

public:

	CSceneNode(CStrID NodeName = CStrID::Empty);
	virtual ~CSceneNode() override;

	virtual bool			IsResourceValid() const override { OK; }

	void					Update(const vector3* pCOIArray, UPTR COICount);
	bool                    UpdateTransform();

	PSceneNode				Clone(CSceneNode* pNewParent = nullptr, bool CloneChildren = true);
	void					RemoveFromParent() { if (pParent) pParent->RemoveChild(*this); }

	bool					AcceptVisitor(INodeVisitor& Visitor);

	CStrID					GetName() const { return Name; }

	CSceneNode*				CreateChild(CStrID ChildName, bool Replace = false);
	CSceneNode*				CreateNodeChain(const char* pPath);
	bool					AddChild(CStrID ChildName, CSceneNode& Node, bool Replace = false);
	void					RemoveChild(CSceneNode& Node);
	void					RemoveChild(UPTR Idx);
	void					RemoveChild(CStrID ChildName);
	CSceneNode*				GetParent() const { return pParent; }
	UPTR					GetChildCount() const { return Children.size(); }
	CSceneNode*				GetChild(UPTR Idx) const { return Children[Idx]; }
	CSceneNode*				GetChild(CStrID ChildName) const;
	CSceneNode*				FindNodeByPath(const char* pPath) const;
	CSceneNode*				FindLastNodeAtPath(const char* pPath, char const*& pUnresolvedPathPart) const;

	bool					AddAttribute(CNodeAttribute& Attr);
	void					RemoveAttribute(CNodeAttribute& Attr);
	void					RemoveAttribute(UPTR Idx);
	UPTR					GetAttributeCount() const { return Attrs.size(); }
	CNodeAttribute*			GetAttribute(IPTR Idx) const { return Attrs[Idx]; }
	template<class T> T*	FindFirstAttribute() const;

	bool					IsRoot() const { return !pParent; }
	bool					IsChild(const CSceneNode* pParentNode) const;
	bool					IsActiveSelf() const { return Flags.Is(SelfActive); }
	bool					IsActive() const { return Flags.Is(EffectivelyActive); }
	void					SetActive(bool Enable) { Flags.SetTo(SelfActive, Enable); UpdateActivity(); }
	bool					IsLocalTransformDirty() const { return Flags.Is(LocalTransformDirty); }
	bool					IsWorldTransformDirty() const { return Flags.Is(WorldTransformDirty); }
	U32						GetTransformVersion() const { n_assert_dbg(!IsWorldTransformDirty()); return TransformVersion; }

	void					SetLocalPosition(const vector3& Value);
	void					SetLocalRotation(const quaternion& Value);
	void					SetLocalScale(const vector3& Value);
	void					SetLocalTransform(const Math::CTransform& Value);
	void					SetLocalTransform(const matrix44& Value);
	void					SetWorldPosition(const vector3& Value);
	void					SetWorldTransform(const matrix44& Value);

	const vector3&			GetLocalPosition() const { n_assert_dbg(!IsLocalTransformDirty()); return LocalTfm.Translation; }
	const quaternion&		GetLocalRotation() const { n_assert_dbg(!IsLocalTransformDirty()); return LocalTfm.Rotation; }
	const vector3&			GetLocalScale() const { n_assert_dbg(!IsLocalTransformDirty()); return LocalTfm.Scale; }
	const Math::CTransform&	GetLocalTransform() const { n_assert_dbg(!IsLocalTransformDirty()); return LocalTfm; }
	matrix44			    GetLocalMatrix() const { n_assert_dbg(!IsLocalTransformDirty()); matrix44 m; LocalTfm.ToMatrix(m); return m; }
	const matrix44&			GetWorldMatrix() const { n_assert_dbg(!IsWorldTransformDirty()); return WorldMatrix; }
	const vector3&			GetWorldPosition() const { n_assert_dbg(!IsWorldTransformDirty()); return WorldMatrix.Translation(); }
};

inline CSceneNode* CSceneNode::FindNodeByPath(const char* pPath) const
{
	const char* pUnprocessed;
	CSceneNode* pNode = FindLastNodeAtPath(pPath, pUnprocessed);
	return pUnprocessed ? nullptr : pNode;
}
//---------------------------------------------------------------------

template<class T> inline T* CSceneNode::FindFirstAttribute() const
{
	for (const auto& Attr : Attrs)
		if (auto Casted = Attr->As<T>()) return Casted;
	return nullptr;
}
//---------------------------------------------------------------------

}
