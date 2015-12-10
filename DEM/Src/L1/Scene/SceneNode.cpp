#include "SceneNode.h"

#include <Scene/NodeController.h>

namespace Scene
{

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
// then leave scene graph branch, perform physics simulation and finally update deffered rigidbody-controlled nodes.
// NB: All deffered nodes are either processed or not, so there are at most two steps of scene graph updating -
// 1st update down to first deffered node (non-inclusive) in each branch, 2nd update all nodes not updated in a 1st step.
void CSceneNode::UpdateTransform(const vector3* pCOIArray, DWORD COICount,
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

	// LOD attrs may disable some children, so process attrs before children
	for (int i = 0; i < Attrs.GetCount(); ++i)
		if (Attrs[i]->IsActive())
			Attrs[i]->Update(pCOIArray, COICount);

	for (int i = 0; i < Children.GetCount(); ++i)
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

CSceneNode* CSceneNode::GetChild(const char* pPath, bool Create)
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

void CSceneNode::RemoveAttribute(DWORD Idx)
{
	n_assert(Idx < (DWORD)Attrs.GetCount());
	CNodeAttribute& Attr = *Attrs[Idx];
	Attr.OnDetachFromNode();
	Attrs.RemoveAt(Idx);
}
//---------------------------------------------------------------------

}