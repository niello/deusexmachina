#include "SceneNode.h"

#include <gfx2/ngfxserver2.h>

namespace Scene
{
//ImplementRTTI(Scripting::CScriptObject, Core::CRefCounted);
//ImplementFactory(Scripting::CScriptObject);

//CSceneNode::~CSceneNode()
//{
//}
////---------------------------------------------------------------------

void CSceneNode::UpdateTransform()
{
	//???use methods SetLocalTfm, SetGlobalTfm by external systems instead?
	// Run transform controller here
	// Skip local tfm setup if controller set global (world) transform directly

	// Entity transform and node transform ARE NOT the same!
	// Entity transform typically corresponds to it's root node transform.

	// Can extract scale of bone nodes to bone info and so force skeleton nodes
	// to be unscaled. Scale will be applied on skin matrix calculation.

	//!!!NB - completely unscaled nodes/meshes can be rendered through dual quat skinning!

	matrix44 MLocalTfm;
	bool LocalTfmChanged = true;
	if (LocalTfmChanged) //???IsDirty?
	{
		MLocalTfm = Tfm.ToMatrix();
		LocalTfmChanged = false;
	}

	//!!!need optimization!
	GlobalTfm = MLocalTfm;
	if (Parent.isvalid()) GlobalTfm.mult_simple(Parent->GetWorldTransform()); //???or parent matrix as arg?

	for (int i = 0; i < Child.Size(); ++i)
		Child.ValueAtIndex(i)->UpdateTransform();

	for (int i = 0; i < Attrs.Size(); ++i)
		Attrs[i]->Update();
}
//---------------------------------------------------------------------

//???rename to UpdateShaderParams?
void CSceneNode::PrepareToRender()
{
	for (int i = 0; i < Attrs.Size(); ++i)
		Attrs[i]->PrepareToRender();
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
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, GlobalTfm, ColorX);
	lines[1].x = 0.f;
	lines[1].y = 1.f;
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, GlobalTfm, ColorY);
	lines[1].y = 0.f;
	lines[1].z = 1.f;
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, GlobalTfm, ColorZ);

	for (int i = 0; i < Child.Size(); ++i)
		Child.ValueAtIndex(i)->RenderDebug();
}
//---------------------------------------------------------------------

PSceneNode CSceneNode::CreateChild(CStrID ChildName)
{
	//???!!!SceneSrv->CreateSceneNode?!
	PSceneNode Node = n_new(CSceneNode);
	Node->Name = ChildName;
	Node->Parent = this;
	Child.Add(ChildName, Node);
	return Node;
}
//---------------------------------------------------------------------

PSceneNode CSceneNode::GetChild(LPCSTR Path, bool Create)
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

}