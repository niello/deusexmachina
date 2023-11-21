#pragma once
#include <Core/Object.h>
#include <Data/StringID.h>
#include <Data/Flags.h>
#include <Math/Matrix44.h>
#include <rtm/qvvf.h>

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

class CSceneNode: public ::Core::CObject
{
public:

	inline static constexpr U32 DIRTY_TRANSFORM_VERSION = 0; // Node with updated transformation never has this version. Use to force update.

protected:

	enum
	{
		SelfActive          = 0x01, // Node itself is active
		EffectivelyActive   = 0x02, // Node is effectively active, meaning it has no inactive parents
		LocalTransformDirty = 0x04,
		WorldTransformDirty = 0x08
		// TODO: add LockTransform? (now locked through static pose)
	};

	rtm::qvvf                   LocalTfm = rtm::qvv_identity();
	rtm::matrix3x4f             WorldMatrix = rtm::matrix_identity();

	CStrID						Name;
	CSceneNode*                 pParent = nullptr;
	std::vector<PSceneNode>     Children; // Always sorted by ID
	std::vector<PNodeAttribute> Attrs;

	Data::CFlags				Flags;
	U32                         TransformVersion = DIRTY_TRANSFORM_VERSION + 1;
	U32                         LastParentTransformVersion = DIRTY_TRANSFORM_VERSION;

	void					UpdateInternal(const rtm::vector4f* pCOIArray, UPTR COICount);
	void					UpdateWorldTransform();
	void					UpdateLocalTransform();

	void                    SetParent(CSceneNode* pNewParent);
	void                    UpdateActivity(bool OnDetach = false);

	DEM_FORCE_INLINE void IncrementTransformVersion() noexcept { if (++TransformVersion == DIRTY_TRANSFORM_VERSION) ++TransformVersion; }

public:

	CSceneNode(CStrID NodeName = CStrID::Empty);
	virtual ~CSceneNode() override;

	void					Update(const rtm::vector4f* pCOIArray, UPTR COICount);
	bool                    UpdateTransform();

	PSceneNode				Clone(CSceneNode* pNewParent = nullptr, bool CloneChildren = true);
	void					RemoveFromParent() { if (pParent) pParent->RemoveChild(*this); }

	bool                    Visit(const std::function<bool(CSceneNode& Node)>& Visitor);

	CStrID					GetName() const { return Name; }

	CSceneNode*				CreateChild(CStrID ChildName, bool Replace = false);
	CSceneNode*				CreateNodeChain(const char* pPath);
	bool					AddChild(CStrID ChildName, PSceneNode Node, bool Replace = false);
	bool                    AddChildAtPath(CStrID ChildName, std::string_view Path, PSceneNode Node, bool Replace = false);
	void					RemoveChild(CSceneNode& Node);
	void					RemoveChild(UPTR Idx);
	void					RemoveChild(CStrID ChildName);
	CSceneNode*				GetParent() const { return pParent; }
	UPTR					GetChildCount() const { return Children.size(); }
	CSceneNode*				GetChild(UPTR Idx) const { return Children[Idx]; }
	CSceneNode*				GetChild(CStrID ChildName) const;
	CSceneNode*				GetChildRecursively(CStrID ChildName) const;
	CSceneNode*				FindNodeByPath(const char* pPath) const;
	CSceneNode*				FindLastNodeAtPath(const char* pPath, char const*& pUnresolvedPathPart) const;

	bool					AddAttribute(CNodeAttribute& Attr);
	void					RemoveAttribute(CNodeAttribute& Attr);
	void					RemoveAttribute(UPTR Idx);
	UPTR					GetAttributeCount() const { return Attrs.size(); }
	CNodeAttribute*			GetAttribute(IPTR Idx) const { return Attrs[Idx]; }
	template<class T> T*	FindFirstAttribute() const;

	bool					IsRoot() const { return !pParent; }
	bool					IsChildOf(const CSceneNode* pParentNode) const;
	bool					IsActiveSelf() const { return Flags.Is(SelfActive); }
	bool					IsActive() const { return Flags.Is(EffectivelyActive); }
	void					SetActive(bool Enable) { Flags.SetTo(SelfActive, Enable); UpdateActivity(); }
	bool					IsLocalTransformDirty() const { return Flags.Is(LocalTransformDirty); }
	bool					IsWorldTransformDirty() const { return Flags.Is(WorldTransformDirty); }
	U32						GetTransformVersion() const { n_assert_dbg(!IsWorldTransformDirty()); return TransformVersion; }

	void					SetLocalPosition(const rtm::vector4f& Value);
	void					SetLocalRotation(const rtm::quatf& Value);
	void					SetLocalScale(const rtm::vector4f& Value);
	void					SetLocalTransform(const rtm::qvvf& Value) { LocalTfm = Value; Flags.Clear(LocalTransformDirty); Flags.Set(WorldTransformDirty); }
	void					SetLocalTransform(const rtm::matrix3x4f& Value);
	void					SetWorldPosition(const rtm::vector4f& Value);
	void					SetWorldTransform(const rtm::matrix3x4f& Value);

	rtm::vector4f			GetLocalPosition() const { n_assert_dbg(!IsLocalTransformDirty()); return LocalTfm.translation; }
	rtm::quatf				GetLocalRotation() const { n_assert_dbg(!IsLocalTransformDirty()); return LocalTfm.rotation; }
	rtm::vector4f			GetLocalScale() const { n_assert_dbg(!IsLocalTransformDirty()); return LocalTfm.scale; }
	const rtm::qvvf&        GetLocalTransform() const { n_assert_dbg(!IsLocalTransformDirty()); return LocalTfm; }
	//matrix44			    GetLocalMatrix() const { n_assert_dbg(!IsLocalTransformDirty()); matrix44 m; LocalTfm.ToMatrix(m); return m; }
	const rtm::matrix3x4f&  GetWorldMatrix() const { n_assert_dbg(!IsWorldTransformDirty()); return WorldMatrix; }
	rtm::vector4f			GetWorldPosition() const { n_assert_dbg(!IsWorldTransformDirty()); return WorldMatrix.w_axis; }
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

// TODO: if make templated for Callable bool(CSceneNode&), will be faster? Will passing callable to children prevent inlining?
inline bool CSceneNode::Visit(const std::function<bool(CSceneNode& Node)>& Visitor)
{
	if (!Visitor(*this)) FAIL;
	for (const auto& Child : Children)
		if (!Child->Visit(Visitor)) FAIL;
	OK;
}
//---------------------------------------------------------------------

}
