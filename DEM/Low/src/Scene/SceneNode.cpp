#include "SceneNode.h"
#include <Scene/NodeAttribute.h>
#include <Scene/NodeVisitor.h>
#include <Data/StringTokenizer.h>

namespace Scene
{
constexpr UPTR MAX_NODE_NAME_LEN = 64;
const std::string ParentToken("^");

CSceneNode::CSceneNode(CStrID NodeName)
	: Name(NodeName)
	, Flags(Active) // All transforms are identity and considered valid
{
}
//---------------------------------------------------------------------

CSceneNode::~CSceneNode()
{
	// Destroy children first
	Children.clear();

	for (const auto& Attr : Attrs)
		Attr->OnDetachFromScene();
	Attrs.clear();
}
//---------------------------------------------------------------------

void CSceneNode::Update(const vector3* pCOIArray, UPTR COICount)
{
	// LOD attrs may disable some children, so process attributes before children
	for (const auto& Attr : Attrs)
		if (Attr->IsActive())
			Attr->Update(pCOIArray, COICount);

	for (const auto& Child : Children)
		if (Child->IsActive())
			Child->Update(pCOIArray, COICount);
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalPosition(const vector3& Value)
{
	// Update before partial change
	if (IsLocalTransformDirty()) UpdateLocalTransform();

	// Save us from recalculating unchanged hierarchy (static poses, constant animation tracks)
	if (LocalTfm.Translation == Value) return;

	LocalTfm.Translation = Value;

	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
	++TransformVersion;
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalRotation(const quaternion& Value)
{
	// Update before partial change
	if (IsLocalTransformDirty()) UpdateLocalTransform();

	// Save us from recalculating unchanged hierarchy (static poses, constant animation tracks)
	if (LocalTfm.Rotation == Value) return;

	LocalTfm.Rotation = Value;

	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
	++TransformVersion;
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalScale(const vector3& Value)
{
	// Update before partial change
	if (IsLocalTransformDirty()) UpdateLocalTransform();

	// Save us from recalculating unchanged hierarchy (static poses, constant animation tracks)
	if (LocalTfm.Scale == Value) return;

	LocalTfm.Scale = Value;

	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
	++TransformVersion;
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalTransform(const Math::CTransform& Value)
{
	LocalTfm = Value;
	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
	++TransformVersion;
}
//---------------------------------------------------------------------

void CSceneNode::SetLocalTransform(const matrix44& Value)
{
	LocalTfm.FromMatrix(Value);
	Flags.Clear(LocalTransformDirty);
	Flags.Set(WorldTransformDirty);
	++TransformVersion;
}
//---------------------------------------------------------------------

void CSceneNode::SetWorldPosition(const vector3& Value)
{
	// Update before partial change
	if (IsWorldTransformDirty()) UpdateWorldTransform();

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
		const matrix44& ParentWorld = pParent->GetWorldMatrix();

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

// NB: world transform doesn't change, so TransformVersion isn't updated
void CSceneNode::UpdateLocalTransform()
{
	if (pParent)
	{
		const matrix44& ParentWorld = pParent->GetWorldMatrix();

		if (!IsLocalTransformDirty() && pParent->GetTransformVersion() == LastParentTransformVersion) return;

		matrix44 LocalMatrix;
		ParentWorld.invert_simple(LocalMatrix);
		LocalMatrix.mult_simple(WorldMatrix);
		LocalTfm.FromMatrix(LocalMatrix);

		LastParentTransformVersion = pParent->GetTransformVersion();
	}
	else
	{
		if (!IsLocalTransformDirty()) return;
		LocalTfm.FromMatrix(WorldMatrix);
	}

	// It is strange to update already valid local transform from an invalid world
	n_assert_dbg(!IsWorldTransformDirty());

	Flags.Clear(LocalTransformDirty | WorldTransformDirty);
}
//---------------------------------------------------------------------

PSceneNode CSceneNode::Clone(bool CloneChildren)
{
	PSceneNode ClonedNode = n_new(CSceneNode(Name));
	ClonedNode->SetLocalTransform(LocalTfm);

	ClonedNode->Attrs.reserve(Attrs.size());
	for (const auto& Attr : Attrs)
		ClonedNode->AddAttribute(*Attr->Clone());

	const auto ChildCount = Children.size();
	if (CloneChildren && ChildCount)
	{
		ClonedNode->Children.reserve(ChildCount);
		for (const auto& Child : Children)
		{
			PSceneNode ClonedChild = Child->Clone(true);
			ClonedChild->SetParent(ClonedNode.Get());
			ClonedNode->Children.push_back(ClonedChild);
		}
		std::sort(ClonedNode->Children.begin(), ClonedNode->Children.end(), [](const PSceneNode& a, const PSceneNode& b) { return a->GetName() < b->GetName(); });
	}

	if (IsActive())
		ClonedNode->Activate();
	else
		ClonedNode->Deactivate();

	return ClonedNode;
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::CreateChild(CStrID ChildName)
{
	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	if (It != Children.end() && (*It)->GetName() == ChildName) return *It;

	//!!!USE POOL!
	PSceneNode Node = n_new(CSceneNode)(ChildName);
	Node->SetParent(this);
	Children.insert(It, Node);
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

bool CSceneNode::AddChild(CStrID ChildName, CSceneNode& Node, bool Replace)
{
	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	if (!Replace && It != Children.end() && (*It)->GetName() == ChildName) return false;

	if (ChildName) Node.Name = ChildName;
	Node.SetParent(this);
	Children.insert(It, &Node);
	return true;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveChild(CSceneNode& Node)
{
	if (Node.pParent != this) return;

	auto It = std::find(Children.begin(), Children.end(), &Node);
	if (It != Children.end())
	{
		Node.OnDetachFromScene();
		Children.erase(It);
		Node.SetParent(nullptr);
	}
}
//---------------------------------------------------------------------

void CSceneNode::RemoveChild(UPTR Idx)
{
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveChild(CStrID ChildName)
{
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::GetChild(CStrID ChildName) const
{
	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	return (It != Children.end() && (*It)->GetName() == ChildName) ? (*It).Get() : nullptr;
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
	if (!Attr.OnAttachToNode(this)) FAIL;
	Attrs.push_back(&Attr);
	OK;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttribute(CNodeAttribute& Attr)
{
	if (Attr.GetNode() != this) return;

	Attr.OnDetachFromScene();
	Attr.OnDetachFromNode();

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
	Attr.OnDetachFromScene();
	Attr.OnDetachFromNode();

	// Don't care about order
	Attrs[Idx] = std::move(Attrs.back());
	Attrs.pop_back();
}
//---------------------------------------------------------------------

void CSceneNode::OnDetachFromScene()
{
	for (const auto& Child : Children)
		Child->OnDetachFromScene();

	for (const auto& Attr : Attrs)
		Attr->OnDetachFromScene();
}
//---------------------------------------------------------------------

void CSceneNode::SetParent(CSceneNode* pNewParent)
{
	pParent = pNewParent;
	if (pParent) LastParentTransformVersion = pParent->GetTransformVersion() - 1;
}
//---------------------------------------------------------------------

bool CSceneNode::AcceptVisitor(INodeVisitor& Visitor)
{
	if (!Visitor.Visit(*this)) FAIL;
	for (const auto& Child : Children)
		if (!Child->AcceptVisitor(Visitor)) FAIL;
	OK;
}
//---------------------------------------------------------------------

}