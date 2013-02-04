#include "SceneNode.h"

#include <gfx2/ngfxserver2.h>

namespace Scene
{
//ImplementRTTI(Scripting::CScriptObject, Core::CRefCounted);
//ImplementFactory(Scripting::CScriptObject);

void CSceneNode::Update()
{
	//???use methods SetLocalTfm, SetGlobalTfm by external systems instead?
	// Run transform controller here
	// Skip local tfm setup if controller set global (world) transform directly

	// Entity transform and node transform ARE NOT the same!
	// Entity transform typically corresponds to it's root node transform.

	// Can extract scale of bone nodes to bone info and so force skeleton nodes
	// to be unscaled. Scale will be applied on skin matrix calculation.

	//!!!NB - completely unscaled nodes/meshes can be rendered through dual quat skinning!

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

	// LODGroup attr may disable some children, so process attrs before children
	for (int i = 0; i < Attrs.Size(); ++i)
		if (Attrs[i]->IsActive())
			Attrs[i]->Update();

	for (int i = 0; i < Child.Size(); ++i)
		if (Child.ValueAtIndex(i)->IsActive())
			Child.ValueAtIndex(i)->Update();
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
	Attrs.Append(&Attr);
	Attr.pNode = this;
	Attr.OnAdd();
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