#include "SceneNode.h"

#include <Scene/NodeController.h>

namespace Scene
{

CSceneNode::~CSceneNode()
{
	Children.Clear();
	for (CArray<PNodeAttribute>::CIterator It = Attrs.Begin(); It != Attrs.End(); ++It)
		(*It)->OnDetachFromNode();
	Attrs.Clear();
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

	if (Flags.Is(WorldMatrixDirty) || (pParent && pParent->IsWorldMatrixChanged()))
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
			if (Controller->ApplyTo(Tfm)) Flags.Set(LocalMatrixDirty | LocalTransformValid);
		}
		else UpdateWorldMatrix = false;
	}

	if (UpdateWorldMatrix)
	{
		UpdateWorldFromLocal();
		Flags.Set(WorldMatrixUpdated);
	}
	else Flags.Clear(WorldMatrixUpdated);

	for (int i = 0; i < Children.GetCount(); ++i)
		if (Children.ValueAt(i)->IsActive())
			Children.ValueAt(i)->UpdateLocalSpace(UpdateWorldMatrix);
}
//---------------------------------------------------------------------

// After UpdateLocalSpace() provided possible constraints etc to physics, and simulation
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
			else Flags.Clear(LocalTransformValid);
		}
		else Flags.Clear(WorldMatrixChanged);
	}
	else if (!Flags.Is(WorldMatrixUpdated)) UpdateWorldFromLocal();

	//???maybe move LOD completely into rendering and other attrs? anyway, AI, Render, Animation LODs are different
	// Here only general scenegraph LOD should work, which manages activity of scene graph parts
	// (not exactly a scene graph, but a dynamic scene manager which loads-unloads parts of the world)
	// for this LOD to work node must have a parameter of distance to enable-disable. Store/pass viewer position
	// into the scene graph or manage it by a View, externally?

	// LODGroup attr may disable some children, so process attrs before children
	for (int i = 0; i < Attrs.GetCount(); ++i)
		if (Attrs[i]->IsActive())
			Attrs[i]->Update();

	for (int i = 0; i < Children.GetCount(); ++i)
		if (Children.ValueAt(i)->IsActive())
			Children.ValueAt(i)->UpdateWorldSpace();
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

CSceneNode* CSceneNode::GetChild(LPCSTR pPath, bool Create)
{
	if (!pPath || !*pPath) return this;

	const DWORD MAX_NODE_NAME = 64;
	char Name[MAX_NODE_NAME];
	const char* pSrcCurr = pPath;
	char* pDstCurr = Name;
	while (*pSrcCurr != '.' && *pSrcCurr)
	{
		*pDstCurr++ = *pSrcCurr++;
		n_assert(pDstCurr < Name + MAX_NODE_NAME);
	}
	n_assert_dbg(pDstCurr > Name + 1);
	*pDstCurr = 0;
	while (*pSrcCurr == '.') ++pSrcCurr;

	PSceneNode SelChild;

	CStrID NameID(Name);
	int Idx = Children.FindIndex(NameID);
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
	if (!Attr.OnAttachToNode(this)) FAIL;
	Attrs.Add(&Attr);
	OK;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttr(CNodeAttribute& Attr)
{
	n_assert(Attr.GetNode() == this);
	Attr.OnDetachFromNode();
	Attrs.RemoveByValue(&Attr);
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttr(DWORD Idx)
{
	n_assert(Idx < (DWORD)Attrs.GetCount());
	CNodeAttribute& Attr = *Attrs[Idx];
	Attr.OnDetachFromNode();
	Attrs.RemoveAt(Idx);
}
//---------------------------------------------------------------------

bool CSceneNode::SetController(CNodeController* pCtlr)
{
	if (Controller.GetUnsafe() == pCtlr) OK;
	if (pCtlr && pCtlr->IsAttachedToNode()) FAIL;

	if (Controller.IsValid())
	{
		n_assert(Controller->GetNode() == this);
		Controller->OnDetachFromNode();
	}

	if (pCtlr && !pCtlr->OnAttachToNode(this)) FAIL;

	Controller = pCtlr;

	OK;
}
//---------------------------------------------------------------------

}