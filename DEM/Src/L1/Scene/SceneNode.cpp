#include "SceneNode.h"

#include <gfx2/ngfxserver2.h>

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
	//!!!be careful with flags!
	//!!! Restore LS from WS using parent WS
}
//---------------------------------------------------------------------

// Update local transform of the node, if it has local controller.
// Also try to update world matrix of this node to provide correct world matrix to children
// and possibly as a constraint to the physics simulation. Once we meet node with a world
// controller, we update only local transform, because we can't rely on parent world matrix
// which will be calculated by physics. UpdateWorldSpace is called after the physics and
// there we can finish updating dependent parts of the hierarchy.
void CSceneNode::UpdateLocalSpace(bool UpdateWorldMatrix)
{
	if (Controller.isvalid() && Controller->IsActive())
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

	for (int i = 0; i < Child.Size(); ++i)
		if (Child.ValueAtIndex(i)->IsActive())
			Child.ValueAtIndex(i)->UpdateLocalSpace(UpdateWorldMatrix);
}
//---------------------------------------------------------------------

// After UpdateLocalSpace provided possible constraints etc to physics, and simulation
// was performed, we can finally update world-controlled nodes and their children.
// After world transform is up-to-date, we update scene node attributes.
void CSceneNode::UpdateWorldSpace()
{
	if (Controller.isvalid() && Controller->IsActive() && !Controller->IsLocalSpace())
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
	for (int i = 0; i < Attrs.Size(); ++i)
		if (Attrs[i]->IsActive())
			Attrs[i]->Update();

	for (int i = 0; i < Child.Size(); ++i)
		if (Child.ValueAtIndex(i)->IsActive())
			Child.ValueAtIndex(i)->UpdateWorldSpace();
}
//---------------------------------------------------------------------

void CSceneNode::RenderDebug()
{
	//!!!To some DebugRenderer / ShapeRenderer as method DrawCoordFrame(matrix44)!
	static const vector4 ColorX(1.0f, 0.0f, 0.0f, 1.0f);
	static const vector4 ColorY(0.0f, 1.0f, 0.0f, 1.0f);
	static const vector4 ColorZ(0.0f, 0.0f, 1.0f, 1.0f);

	nFixedArray<vector3> lines(2);
	lines[1].x = 1.f;
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, WorldMatrix, ColorX);
	lines[1].x = 0.f;
	lines[1].y = 1.f;
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, WorldMatrix, ColorY);
	lines[1].y = 0.f;
	lines[1].z = 1.f;
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, WorldMatrix, ColorZ);

	for (int i = 0; i < Child.Size(); ++i)
		Child.ValueAtIndex(i)->RenderDebug();
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
	n_assert(pDstCurr > Name + 1);
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

bool CSceneNode::AddAttr(CSceneNodeAttr& Attr)
{
	if (Attr.pNode) FAIL;
	Attr.pNode = this;
	if (!Attr.OnAdd())
	{
		Attr.pNode = NULL;
		FAIL;
	}
	Attrs.Append(&Attr);
	OK;
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttr(CSceneNodeAttr& Attr)
{
	n_assert(Attr.pNode == this);
	Attr.OnRemove();
	Attr.pNode = NULL;
	Attrs.RemoveByValue(&Attr);
}
//---------------------------------------------------------------------

void CSceneNode::RemoveAttr(DWORD Idx)
{
	n_assert(Idx < (DWORD)Attrs.Size());
	CSceneNodeAttr& Attr = *Attrs[Idx];
	Attr.OnRemove();
	Attr.pNode = NULL;
	Attrs.Erase(Idx);
}
//---------------------------------------------------------------------

}