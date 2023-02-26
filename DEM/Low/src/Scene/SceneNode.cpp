#include "SceneNode.h"
#include <Scene/NodeAttribute.h>
#include <Data/StringTokenizer.h>

namespace Scene
{
constexpr UPTR MAX_NODE_NAME_LEN = 64;
const std::string ParentToken("^");

// All transforms are identity and considered valid, so LocalTransformDirty & WorldTransformDirty aren't set.
// Root node is assumed effectively inactive until the first Update from user, so EffectivelyActive is not set.
CSceneNode::CSceneNode(CStrID NodeName)
	: Name(NodeName)
	, Flags(SelfActive)
{
}
//---------------------------------------------------------------------

// NB: strong refs to children and attributes may exist, can't assume them being destroyed here
CSceneNode::~CSceneNode()
{
	// Detach children before attributes
	for (const auto& Child : Children)
		Child->SetParent(nullptr);

	for (const auto& Attr : Attrs)
		Attr->SetNode(nullptr);
}
//---------------------------------------------------------------------

void CSceneNode::UpdateInternal(const vector3* pCOIArray, UPTR COICount)
{
	UpdateWorldTransform();

	for (const auto& Attr : Attrs)
		if (Attr->IsActive())
			Attr->UpdateBeforeChildren(pCOIArray, COICount);

	for (const auto& Child : Children)
		if (Child->IsActive())
			Child->UpdateInternal(pCOIArray, COICount);

	for (const auto& Attr : Attrs)
		if (Attr->IsActive())
			Attr->UpdateAfterChildren(pCOIArray, COICount);
}
//---------------------------------------------------------------------

void CSceneNode::Update(const vector3* pCOIArray, UPTR COICount)
{
	// The first time an active root node is updated by user, it must be effectively activated.
	// This is the difference between a scene root and a detached part of the scene.
	if (IsActiveSelf() && !pParent) UpdateActivity();

	if (!IsActive())
	{
		::Sys::Error("CSceneNode::Update() > can't update an inactive node");
		return;
	}

	UpdateInternal(pCOIArray, COICount);
}
//---------------------------------------------------------------------

// Recalculates the hierarchy up to the root and updates all current node transforms.
// May be slow, use on initialization or when absolutely necessary.
bool CSceneNode::UpdateTransform()
{
	// Do we have invalid transform?
	bool Changed = Flags.IsAny(LocalTransformDirty | WorldTransformDirty);

	// Update parent, its transform might change too
	if (pParent) Changed |= pParent->UpdateTransform();

	// All is up to date
	if (!Changed) FAIL;

	// Update our transforms based on updated parent transform.
	// NB: world transform is kept intact if it was set directly and local is not updated yet.
	if (IsLocalTransformDirty()) UpdateLocalTransform();
	else UpdateWorldTransform();

	OK;
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalPosition(const vector3& Value)
{
	// Update before partial change
	if (IsLocalTransformDirty()) UpdateTransform();

	// Save us from recalculating unchanged hierarchy (static poses, constant animation tracks)
	if (LocalTfm.Translation == Value) return;

	LocalTfm.Translation = Value;

	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalRotation(const quaternion& Value)
{
	// Update before partial change
	if (IsLocalTransformDirty()) UpdateTransform();

	// Save us from recalculating unchanged hierarchy (static poses, constant animation tracks)
	if (LocalTfm.Rotation == Value) return;

	LocalTfm.Rotation = Value;

	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalScale(const vector3& Value)
{
	// Update before partial change
	if (IsLocalTransformDirty()) UpdateTransform();

	// Save us from recalculating unchanged hierarchy (static poses, constant animation tracks)
	if (LocalTfm.Scale == Value) return;

	LocalTfm.Scale = Value;

	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalTransform(const Math::CTransform& Value)
{
	LocalTfm = Value;
	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalTransform(const matrix44& Value)
{
	LocalTfm.FromMatrix(Value);
	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
}
//---------------------------------------------------------------------

void CSceneNode::SetWorldPosition(const vector3& Value)
{
	// Update before partial change
	if (IsWorldTransformDirty()) UpdateTransform();

	// Save us from recalculating unchanged hierarchy (static poses, constant animation tracks)
	if (WorldMatrix.Translation() == Value) return;

	WorldMatrix.Translation() = Value;

	Flags.Set(LocalTransformDirty);
	Flags.Clear(WorldTransformDirty);
	++TransformVersion;
}
//---------------------------------------------------------------------

void CSceneNode::SetWorldTransform(const matrix44& Value)
{
	WorldMatrix = Value;
	Flags.Set(LocalTransformDirty);
	Flags.Clear(WorldTransformDirty);
	++TransformVersion;
}
//---------------------------------------------------------------------

void CSceneNode::UpdateWorldTransform()
{
	if (pParent)
	{
		if (!IsWorldTransformDirty() && pParent->GetTransformVersion() == LastParentTransformVersion) return;

		LocalTfm.ToMatrix(WorldMatrix);
		WorldMatrix.mult_simple(pParent->GetWorldMatrix());

		LastParentTransformVersion = pParent->GetTransformVersion();
	}
	else
	{
		if (!IsWorldTransformDirty()) return;
		LocalTfm.ToMatrix(WorldMatrix);
	}

	++TransformVersion;

	// It is strange to update already valid world transform from an invalid local
	n_assert_dbg(!IsLocalTransformDirty());

	Flags.Clear(LocalTransformDirty | WorldTransformDirty);
}
//---------------------------------------------------------------------

void CSceneNode::UpdateLocalTransform()
{
	// It is strange to update already valid local transform from an invalid world
	n_assert_dbg(!IsWorldTransformDirty());

	if (pParent)
	{
		if (!IsLocalTransformDirty() && pParent->GetTransformVersion() == LastParentTransformVersion) return;

		matrix44 LocalMatrix;
		pParent->GetWorldMatrix().invert_simple(LocalMatrix);
		LocalMatrix.mult_simple(WorldMatrix);
		LocalTfm.FromMatrix(LocalMatrix);

		LastParentTransformVersion = pParent->GetTransformVersion();
	}
	else
	{
		if (!IsLocalTransformDirty()) return;
		LocalTfm.FromMatrix(WorldMatrix);
	}

	Flags.Clear(LocalTransformDirty | WorldTransformDirty);
}
//---------------------------------------------------------------------

PSceneNode CSceneNode::Clone(CSceneNode* pNewParent, bool CloneChildren)
{
	PSceneNode ClonedNode = pNewParent ? pNewParent->CreateChild(Name) : n_new(CSceneNode(Name));
	ClonedNode->SetActive(IsActiveSelf());
	ClonedNode->SetLocalTransform(LocalTfm);

	// NB: some of attributes may be intentionally not cloned, e.g. CSkinProcessorAttribute
	ClonedNode->Attrs.reserve(Attrs.size());
	for (const auto& Attr : Attrs)
		if (PNodeAttribute ClonedAttr = Attr->Clone())
			ClonedNode->AddAttribute(*ClonedAttr);

	const auto ChildCount = Children.size();
	if (CloneChildren && ChildCount)
	{
		ClonedNode->Children.reserve(ChildCount);
		for (const auto& Child : Children)
			Child->Clone(ClonedNode.Get(), true);
	}

	return ClonedNode;
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::CreateChild(CStrID ChildName, bool Replace)
{
	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	if (It != Children.end() && (*It)->GetName() == ChildName)
	{
		if (Replace) It = Children.erase(It);
		else return *It;
	}

	PSceneNode Node = n_new(CSceneNode)(ChildName);
	Children.insert(It, Node);
	Node->SetParent(this);
	return Node;
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::CreateNodeChain(const char* pPath)
{
	CSceneNode* pCurrNode = this;

	if (!pPath || !*pPath) return pCurrNode;

	char Buffer[MAX_NODE_NAME_LEN];
	Data::CStringTokenizer StrTok(pPath, Buffer, MAX_NODE_NAME_LEN);
	while (pCurrNode && StrTok.GetNextToken('.'))
	{
		if (StrTok.GetCurrToken() == ParentToken)
			pCurrNode = pCurrNode->GetParent();
		else
			pCurrNode = pCurrNode->CreateChild(CStrID(StrTok.GetCurrToken()));
	}

	return pCurrNode;
}
//---------------------------------------------------------------------

bool CSceneNode::AddChild(CStrID ChildName, PSceneNode Node, bool Replace)
{
	if (!Node) return false;

	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	if (It != Children.end() && (*It)->GetName() == ChildName)
	{
		if (Replace) It = Children.erase(It);
		else return *It;
	}

	if (ChildName) Node->Name = ChildName;
	Children.insert(It, Node);
	Node->SetParent(this);
	return true;
}
//---------------------------------------------------------------------

bool CSceneNode::AddChildAtPath(CStrID ChildName, std::string_view Path, PSceneNode Node, bool Replace)
{
	if (!Node) return false;

	auto pParent = this;

	const auto LastDotPos = Path.rfind('.');
	if (LastDotPos != std::string_view::npos)
	{
		//!!!FIXME: need tokenizer for non-null-terminated strings, on views!
		pParent = CreateNodeChain(std::string(Path.begin(), Path.begin() + LastDotPos).c_str());
		if (!pParent) return false;
	}

	return pParent->AddChild(ChildName, Node, Replace);
}
//---------------------------------------------------------------------

void CSceneNode::RemoveChild(CSceneNode& Node)
{
	if (Node.pParent != this) return;

	auto It = std::find(Children.begin(), Children.end(), &Node);
	if (It != Children.end())
	{
		Node.SetParent(nullptr);
		Children.erase(It);
	}
}
//---------------------------------------------------------------------

void CSceneNode::RemoveChild(UPTR Idx)
{
	if (Idx >= Children.size()) return;

	auto It = Children.begin() + Idx;
	It->Get()->SetParent(nullptr);
	Children.erase(It);
}
//---------------------------------------------------------------------

void CSceneNode::RemoveChild(CStrID ChildName)
{
	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	if (It != Children.end() && (*It)->GetName() == ChildName)
	{
		It->Get()->SetParent(nullptr);
		Children.erase(It);
	}
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::GetChild(CStrID ChildName) const
{
	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	return (It != Children.end() && (*It)->GetName() == ChildName) ? (*It).Get() : nullptr;
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::GetChildRecursively(CStrID ChildName) const
{
	if (auto pChild = GetChild(ChildName)) return pChild;

	for (auto& Child : Children)
		if (auto pChild = Child->GetChildRecursively(ChildName)) return pChild;

	return nullptr;
}
//---------------------------------------------------------------------

bool CSceneNode::IsChild(const CSceneNode* pParentNode) const
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

// NB: no data is changed inside this method, but const_cast is required to return non-const node pointer.
// If pUnresolvedPathPart == nullptr, the whole path was resolved. Otherwise the last existing node is returned.
CSceneNode* CSceneNode::FindLastNodeAtPath(const char* pPath, char const* & pUnresolvedPathPart) const
{
	if (!pPath || !*pPath)
	{
		pUnresolvedPathPart = nullptr;
		return const_cast<CSceneNode*>(this);
	}

	const CSceneNode* pCurrNode = this;
	pUnresolvedPathPart = pPath;

	char Buffer[MAX_NODE_NAME_LEN];
	Data::CStringTokenizer StrTok(pPath, Buffer, MAX_NODE_NAME_LEN);
	while (StrTok.GetNextToken('.'))
	{
		CSceneNode* pNextNode = nullptr;
		if (StrTok.GetCurrToken() == ParentToken)
			pNextNode = pCurrNode->GetParent();
		else
			pNextNode = pCurrNode->GetChild(CStrID(StrTok.GetCurrToken()));

		if (pNextNode)
		{
			pUnresolvedPathPart = StrTok.GetCursor();
			pCurrNode = pNextNode;
		}
		else
		{
			return const_cast<CSceneNode*>(pCurrNode);
		}
	}

	pUnresolvedPathPart = nullptr;
	return const_cast<CSceneNode*>(pCurrNode);
}
//---------------------------------------------------------------------

bool CSceneNode::AddAttribute(CNodeAttribute& Attr)
{
	if (Attr.GetNode()) FAIL;

	Attr.SetNode(this);
	Attrs.push_back(&Attr);
	OK;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttribute(CNodeAttribute& Attr)
{
	if (Attr.GetNode() != this) return;

	Attr.SetNode(nullptr);

	// Don't care about order
	auto It = std::find(Attrs.begin(), Attrs.end(), &Attr);
	if (It != Attrs.end())
	{
		*It = std::move(Attrs.back());
		Attrs.pop_back();
	}
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttribute(UPTR Idx)
{
	if (Idx >= Attrs.size()) return;

	CNodeAttribute& Attr = *Attrs[Idx];

	Attr.SetNode(nullptr);

	// Don't care about order
	Attrs[Idx] = std::move(Attrs.back());
	Attrs.pop_back();
}
//---------------------------------------------------------------------

void CSceneNode::SetParent(CSceneNode* pNewParent)
{
	if (pParent == pNewParent) return;

	pParent = pNewParent;
	if (pParent) LastParentTransformVersion = pParent->TransformVersion - 1;
	UpdateActivity(!pParent);
}
//---------------------------------------------------------------------

// NB: node is always effectively deactivated on detach from parent. Update it if you want to use it as
// a standalone scene root. It will be automatically effectively activated on the first user Update.
void CSceneNode::UpdateActivity(bool OnDetach)
{
	const bool WasActive = IsActive();
	const bool NowActive = IsActiveSelf() && !OnDetach && (!pParent || pParent->IsActive());
	if (WasActive != NowActive)
	{
		Flags.SetTo(EffectivelyActive, NowActive);

		for (const auto& Attr : Attrs)
			Attr->UpdateActivity();

		for (const auto& Child : Children)
			Child->UpdateActivity();
	}
}
//---------------------------------------------------------------------

}
