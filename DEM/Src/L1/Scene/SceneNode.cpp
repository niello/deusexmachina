#include "SceneNode.h"

#include <Render/DebugDraw.h>

namespace Scene
{

void CSceneNode::UpdateWorldFromLocal()
{
	if (Flags.Is(LocalMatrixDirty))
	{
		Tfm.ToMatrix(LocalMatrix);
		Flags.Clear(LocalMatrixDirty);
		Flags.Set(WorldMatrixDirty);
	}

	if (Flags.Is(WorldMatrixDirty) || (pParent && pParent->IsWorldMatrixChanged()))
	{
		if (pParent) WorldMatrix.mult2_simple(LocalMatrix, pParent->WorldMatrix);
		else WorldMatrix = LocalMatrix;
		Flags.Clear(WorldMatrixDirty);
		Flags.Set(WorldMatrixChanged);
	}
	else Flags.Clear(WorldMatrixChanged);
}
//---------------------------------------------------------------------

void CSceneNode::UpdateLocalFromWorld()
{
	if (pParent)
	{
		matrix44 InvParentPos;
		pParent->GetWorldMatrix().invert_simple(InvParentPos);
		LocalMatrix = InvParentPos * WorldMatrix;
	}
	else LocalMatrix = WorldMatrix;
	Tfm.FromMatrix(LocalMatrix);
	Flags.Clear(WorldMatrixDirty | LocalMatrixDirty);
}
//---------------------------------------------------------------------

// Updates local transform of the node, if it has local controller.
// Also tries to update world matrix of this node to provide correct world matrix to children
// and possibly as a constraint to the physics simulation. Once we meet node with a world
// controller, we update only local transform, because we can't rely on parent world matrix
// which will be calculated by physics. UpdateWorldSpace() is called after the physics and
// there we can finish updating dependent parts of the hierarchy.
void CSceneNode::UpdateLocalSpace(bool UpdateWorldMatrix)
{
	if (Controller.IsValid() && Controller->IsActive())
	{
		if (Controller->IsLocalSpace())
		{
			if (Controller->ApplyTo(Tfm)) Flags.Set(LocalMatrixDirty);
		}
		else UpdateWorldMatrix = false;
	}

	if (UpdateWorldMatrix)
	{
		UpdateWorldFromLocal();
		Flags.Set(WorldMatrixUpdated);
	}
	else Flags.Clear(WorldMatrixUpdated);

	for (int i = 0; i < Child.GetCount(); ++i)
		if (Child.ValueAt(i)->IsActive())
			Child.ValueAt(i)->UpdateLocalSpace(UpdateWorldMatrix);
}
//---------------------------------------------------------------------

// After UpdateLocalSpace provided possible constraints etc to physics, and simulation
// was performed, we can finally update world-controlled nodes and their children.
// After world transform is up-to-date, we update scene node attributes.
void CSceneNode::UpdateWorldSpace()
{
	if (Controller.IsValid() && Controller->IsActive() && !Controller->IsLocalSpace())
	{
		Math::CTransformSRT WorldSRT;
		if (Controller->ApplyTo(WorldSRT))
		{
			WorldSRT.ToMatrix(WorldMatrix);
			Flags.Clear(WorldMatrixDirty);
			Flags.Set(WorldMatrixChanged);
			if (Controller->NeedToUpdateLocalSpace()) UpdateLocalFromWorld();
		}
		else Flags.Clear(WorldMatrixChanged);
	}
	else if (!Flags.Is(WorldMatrixUpdated)) UpdateWorldFromLocal();

	// LODGroup attr may disable some children, so process attrs before children
	for (int i = 0; i < Attrs.GetCount(); ++i)
		if (Attrs[i]->IsActive())
			Attrs[i]->Update();

	for (int i = 0; i < Child.GetCount(); ++i)
		if (Child.ValueAt(i)->IsActive())
			Child.ValueAt(i)->UpdateWorldSpace();
}
//---------------------------------------------------------------------

void CSceneNode::RenderDebug()
{
	if (pParent)
		DebugDraw->DrawLine(pParent->WorldMatrix.Translation(), WorldMatrix.Translation(), vector4::White);

	for (int i = 0; i < Child.GetCount(); ++i)
		Child.ValueAt(i)->RenderDebug();
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::CreateChild(CStrID ChildName)
{
	//???!!!SceneSrv->CreateSceneNode?!
	PSceneNode Node = n_new(CSceneNode)(*pScene, ChildName);
	Node->pParent = this;
	Child.Add(ChildName, Node);
	return Node;
}
//---------------------------------------------------------------------

CSceneNode* CSceneNode::GetChild(LPCSTR Path, bool Create)
{
	n_assert(Path && *Path);

	const int MAX_NODE_NAME = 64;
	char Name[MAX_NODE_NAME];
	const char* pSrcCurr = Path;
	char* pDstCurr = Name;
	while (*pSrcCurr != '.' && *pSrcCurr)
	{
		*pDstCurr++ = *pSrcCurr++;
		n_assert(pDstCurr - Name < MAX_NODE_NAME);
	}
	n_assert_dbg(pDstCurr > Name + 1);
	*pDstCurr = 0;
	while (*pSrcCurr == '.') ++pSrcCurr;

	PSceneNode SelChild;

	CStrID NameID(Name);
	int Idx = Child.FindIndex(NameID);
	if (Idx == INVALID_INDEX)
	{
		if (!Create) return NULL;
		SelChild = CreateChild(NameID);
	}
	else SelChild = GetChild(Idx);

	return *pSrcCurr ? SelChild->GetChild(pSrcCurr, Create) : SelChild;
}
//---------------------------------------------------------------------

bool CSceneNode::AddAttr(CNodeAttribute& Attr)
{
	if (Attr.pNode) FAIL;
	Attr.pNode = this;
	if (!Attr.OnAttachToNode(this)) FAIL;
	Attrs.Append(&Attr);
	OK;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttr(CNodeAttribute& Attr)
{
	n_assert(Attr.pNode == this);
	Attr.OnDetachFromNode();
	Attr.pNode = NULL;
	Attrs.RemoveByValue(&Attr);
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttr(DWORD Idx)
{
	n_assert(Idx < (DWORD)Attrs.GetCount());
	CNodeAttribute& Attr = *Attrs[Idx];
	Attr.OnDetachFromNode();
	Attr.pNode = NULL;
	Attrs.EraseAt(Idx);
}
//---------------------------------------------------------------------

bool CSceneNode::SetController(CNodeController* pCtlr)
{
	if (Controller.GetUnsafe() == pCtlr) OK;
	if (pCtlr && pCtlr->pNode) FAIL;

	if (Controller.IsValid())
	{
		n_assert(Controller->pNode == this);
		Controller->OnDetachFromNode();
		Controller->pNode = NULL;
	}

	if (pCtlr)
	{
		pCtlr->pNode = this;
		if (!pCtlr->OnAttachToNode(this))
		{
			pCtlr->pNode = NULL;
			FAIL;
		}
	}

	Controller = pCtlr;

	OK;
}
//---------------------------------------------------------------------

void CSceneNode::SetWorldTransform(const matrix44& Transform)
{
	WorldMatrix = Transform;
	Flags.Set(WorldMatrixChanged);
	UpdateLocalFromWorld();
}
//---------------------------------------------------------------------

}