#include "SceneNode.h"

#include <Scene/NodeController.h>
#include <Data/StringTokenizer.h>

namespace Scene
{
const UPTR MAX_NODE_NAME_LEN = 64;

CSceneNode::~CSceneNode()
{
	Children.Clear();

	if (Controller.IsValidPtr()) Controller->OnDetachFromNode();

	for (CArray<PNodeAttribute>::CIterator It = Attrs.Begin(); It != Attrs.End(); ++It)
		(*It)->OnDetachFromNode();
	Attrs.Clear();
}
//---------------------------------------------------------------------

// Some nodes may be driven by a deffered controller, i.e. a controller that is driven by some closed external system
// like a physics simulation, but is dependent on a parent node transform. Example is a rigid body carried by an
// animated character. So, we update character animation, those providing correct physical constraint position,
// then leave scene graph branch, perform physics simulation and finally update deffered rigid-body-controlled nodes.
// NB: All deffered nodes are either processed or not, so there are at most two steps of scene graph updating -
// 1st update down to first deffered node (not inclusive) in each branch, 2nd update all nodes not updated in a 1st step.
void CSceneNode::UpdateTransform(const vector3* pCOIArray, UPTR COICount,
								 bool ProcessDefferedController, CArray<CSceneNode*>* pOutDefferedNodes)
{
	if (Controller.IsValidPtr() && Controller->IsActive())
	{
		if (!ProcessDefferedController && Controller->IsDeffered())
		{
			if (pOutDefferedNodes) pOutDefferedNodes->Add(this);
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
	for (UPTR i = 0; i < Attrs.GetCount(); ++i)
		if (Attrs[i]->IsActive())
			Attrs[i]->Update(pCOIArray, COICount);

	for (UPTR i = 0; i < Children.GetCount(); ++i)
		if (Children.ValueAt(i)->IsActive())
			Children.ValueAt(i)->UpdateTransform(pCOIArray, COICount, ProcessDefferedController, pOutDefferedNodes);
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
		matrix44 InvParentPos;
		pParent->WorldMatrix.invert_simple(InvParentPos);
		LocalMatrix.mult2_simple(InvParentPos, WorldMatrix);
	}
	else LocalMatrix = WorldMatrix;
	Tfm.FromMatrix(LocalMatrix);
	Flags.Clear(WorldMatrixDirty | LocalMatrixDirty);
	Flags.Set(LocalTransformValid);
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::CreateChild(CStrID ChildName)
{
	//!!!USE POOL!
	PSceneNode Node = n_new(CSceneNode)(ChildName);
	Node->pParent = this;
	Children.Add(ChildName, Node);
	return Node;
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::CreateChildChain(const char* pPath)
{
	CSceneNode* pCurrNode = this;

	if (!pPath || !*pPath) return pCurrNode;

	char Buffer[MAX_NODE_NAME_LEN];
	Data::CStringTokenizer StrTok(pPath, Buffer, MAX_NODE_NAME_LEN);
	while (StrTok.GetNextToken('.'))
		pCurrNode = pCurrNode->CreateChild(CStrID(StrTok.GetCurrToken()));

	return pCurrNode;
}
//---------------------------------------------------------------------

// NB: no data is changed inside this method, but const_cast is required to return non-const node pointer.
// If pUnresolvedPathPart == NULL, the child node is found, return this node. Else the deepest found node is returned.
CSceneNode* CSceneNode::FindDeepestChild(const char* pPath, char const* & pUnresolvedPathPart) const
{
	const CSceneNode* pCurrNode = this;

	if (!pPath || !*pPath)
	{
		pUnresolvedPathPart = NULL;
		return const_cast<CSceneNode*>(pCurrNode);
	}

	pUnresolvedPathPart = pPath;

	char Buffer[MAX_NODE_NAME_LEN];
	Data::CStringTokenizer StrTok(pPath, Buffer, MAX_NODE_NAME_LEN);
	while (StrTok.GetNextToken('.'))
	{
		CStrID ChildID(StrTok.GetCurrToken());
		IPTR Idx = pCurrNode->Children.FindIndex(ChildID);
		if (Idx == INVALID_INDEX) return const_cast<CSceneNode*>(pCurrNode);
		else
		{
			pUnresolvedPathPart = StrTok.GetCursor();
			pCurrNode = pCurrNode->Children.ValueAt(Idx);
		}
	}

	pUnresolvedPathPart = NULL;
	return const_cast<CSceneNode*>(pCurrNode);
}
//---------------------------------------------------------------------

bool CSceneNode::SetController(CNodeController* pCtlr)
{
	if (Controller.GetUnsafe() == pCtlr) OK;
	if (pCtlr && pCtlr->IsAttachedToNode()) FAIL;

	if (Controller.IsValidPtr())
	{
		n_assert(Controller->GetNode() == this);
		Controller->OnDetachFromNode();
	}

	if (pCtlr && !pCtlr->OnAttachToNode(this)) FAIL;

	Controller = pCtlr;

	OK;
}
//---------------------------------------------------------------------

bool CSceneNode::AddAttribute(CNodeAttribute& Attr)
{
	if (!Attr.OnAttachToNode(this)) FAIL;
	Attrs.Add(&Attr);
	OK;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttribute(CNodeAttribute& Attr)
{
	n_assert(Attr.GetNode() == this);
	Attr.OnDetachFromNode();
	Attrs.RemoveByValue(&Attr);
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttribute(UPTR Idx)
{
	n_assert(Idx < Attrs.GetCount());
	CNodeAttribute& Attr = *Attrs[Idx];
	Attr.OnDetachFromNode();
	Attrs.RemoveAt(Idx);
}
//---------------------------------------------------------------------

}