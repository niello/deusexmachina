#include "SceneNode.h"
#include <Scene/NodeAttribute.h>
#include <Scene/NodeController.h>
#include <Scene/NodeVisitor.h>
#include <Data/StringTokenizer.h>

namespace Scene
{
constexpr UPTR MAX_NODE_NAME_LEN = 64;
const std::string ParentToken("^");

CSceneNode::CSceneNode(CStrID NodeName)
	: Name(NodeName)
	, Flags(Active | LocalMatrixDirty | LocalTransformValid)
{
}
//---------------------------------------------------------------------

CSceneNode::~CSceneNode()
{
	// Destroy children first
	Children.clear();

	if (Controller) Controller->OnDetachFromScene();

	for (const auto& Attr : Attrs)
		Attr->OnDetachFromScene();
	Attrs.clear();
}
//---------------------------------------------------------------------

// Some nodes may be driven by a deffered controller, i.e. a controller that is driven by some closed external system
// like a physics simulation, but is dependent on a parent node transform. Example is a rigid body carried by an
// animated character. So, we update character animation, those providing correct physical constraint position,
// then leave scene graph branch, perform physics simulation and finally update deffered rigid-body-controlled nodes.
// NB: All deffered nodes are either processed or not, so there are at most two steps of scene graph updating -
// 1st update down to first deffered node (not inclusive) in each branch, 2nd update all nodes not updated in a 1st step.
void CSceneNode::UpdateTransform(const vector3* pCOIArray, UPTR COICount,
								 bool ProcessDefferedController, std::vector<CSceneNode*>* pOutDefferedNodes)
{
	if (Controller.IsValidPtr() && Controller->IsActive())
	{
		if (!ProcessDefferedController && Controller->IsDeffered())
		{
			if (pOutDefferedNodes) pOutDefferedNodes->push_back(this);
			return;
		}

		if (Controller->IsLocalSpace())
		{
			if (Controller->ApplyTo(Tfm)) Flags.Set(LocalMatrixDirty | LocalTransformValid);
			UpdateWorldFromLocal();
		}
		else
		{
			Math::CTransformSRT WorldSRT;
			if (Controller->ApplyTo(WorldSRT))
			{
				WorldSRT.ToMatrix(WorldMatrix);
				Flags.Clear(WorldMatrixDirty);
				Flags.Set(WorldMatrixChanged);
				if (Controller->NeedToUpdateLocalSpace()) UpdateLocalFromWorld();
				else Flags.Clear(LocalTransformValid);
			}
			else Flags.Clear(WorldMatrixChanged);
		}
	}
	else UpdateWorldFromLocal();

	// LOD attrs may disable some children, so process attributes before children
	for (const auto& Attr : Attrs)
		if (Attr->IsActive())
			Attr->Update(pCOIArray, COICount);

	for (const auto& Child : Children)
		if (Child->IsActive())
			Child->UpdateTransform(pCOIArray, COICount, ProcessDefferedController, pOutDefferedNodes);
}
//---------------------------------------------------------------------

void CSceneNode::UpdateWorldFromLocal()
{
	if (Flags.Is(LocalMatrixDirty))
	{
		Tfm.ToMatrix(LocalMatrix);
		Flags.Clear(LocalMatrixDirty);
		Flags.Set(WorldMatrixDirty);
	}

	if (Flags.Is(WorldMatrixDirty) || (pParent /*&& !pParent->IsRoot()*/ && pParent->IsWorldMatrixChanged()))
	{
		if (pParent) WorldMatrix.mult2_simple(LocalMatrix, pParent->WorldMatrix);
		else WorldMatrix = LocalMatrix;
		Flags.Clear(WorldMatrixDirty);
		Flags.Set(WorldMatrixChanged);
	}
	else Flags.Clear(WorldMatrixChanged);

	Flags.Set(LocalTransformValid);
}
//---------------------------------------------------------------------

void CSceneNode::UpdateLocalFromWorld()
{
	if (pParent)
	{
		matrix44 InvParentTfm;
		pParent->WorldMatrix.invert_simple(InvParentTfm);
		LocalMatrix.mult2_simple(InvParentTfm, WorldMatrix);
	}
	else LocalMatrix = WorldMatrix;
	Tfm.FromMatrix(LocalMatrix);
	Flags.Clear(WorldMatrixDirty | LocalMatrixDirty);
	Flags.Set(LocalTransformValid);
}
//---------------------------------------------------------------------

PSceneNode CSceneNode::Clone(bool CloneChildren)
{
	PSceneNode ClonedNode = n_new(CSceneNode(Name));
	ClonedNode->SetScale(Tfm.Scale);
	ClonedNode->SetRotation(Tfm.Rotation);
	ClonedNode->SetPosition(Tfm.Translation);

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
			ClonedChild->pParent = ClonedNode.Get();
			ClonedNode->Children.push_back(ClonedChild);
		}
		std::sort(ClonedNode->Children.begin(), ClonedNode->Children.end(), [](const PSceneNode& a, const PSceneNode& b) { return a->GetName() < b->GetName(); });
	}

	//???clone controller?

	ClonedNode->Activate(IsActive());
	return ClonedNode;
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::CreateChild(CStrID ChildName)
{
	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	if (It != Children.end() && (*It)->GetName() == ChildName) return *It;

	//!!!USE POOL!
	PSceneNode Node = n_new(CSceneNode)(ChildName);
	Node->pParent = this;
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
	auto It = std::lower_bound(Children.begin(), Children.end(), &Node);
	if (!Replace && It != Children.end() && (*It)->GetName() == ChildName) return false;

	if (ChildName) Node.Name = ChildName;
	Node.pParent = this;
	Children.insert(It, &Node);
	return true;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveChild(CSceneNode& Node)
{
	if (Node.pParent != this) return;

	auto It = std::lower_bound(Children.begin(), Children.end(), &Node);
	if (It != Children.end() && (*It).Get() == &Node)
	{
		Node.OnDetachFromScene();
		Children.erase(It);
		Node.pParent = nullptr;
	}
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

bool CSceneNode::SetController(CNodeController* pCtlr)
{
	if (Controller.Get() == pCtlr) OK;
	if (pCtlr && pCtlr->IsAttachedToNode()) FAIL;

	if (Controller.IsValidPtr())
	{
		n_assert(Controller->GetNode() == this);
		Controller->OnDetachFromScene();
	}

	if (pCtlr && !pCtlr->OnAttachToNode(this)) FAIL;

	Controller = pCtlr;

	OK;
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

	if (Controller.IsValidPtr()) Controller->OnDetachFromScene();

	for (const auto& Attr : Attrs)
		Attr->OnDetachFromScene();
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