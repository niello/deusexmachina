#include "PropSceneNode.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scripting/PropScriptable.h>
#include <Scene/Events/SetTransform.h>
#include <Scene/Model.h>
#include <Physics/NodeAttrCollision.h>
#include <Data/DataArray.h>
#include <Render/DebugDraw.h>
#include <Core/Factory.h>

namespace Scene
{
	bool LoadNodesFromSCN(const CString& FileName, PSceneNode RootNode);
}

namespace Prop
{
__ImplementClass(Prop::CPropSceneNode, 'PSCN', Game::CProperty);
__ImplementPropertyStorage(CPropSceneNode);

IMPL_EVENT_HANDLER_VIRTUAL(OnRenderDebug, CPropSceneNode, OnRenderDebug)

bool CPropSceneNode::InternalActivate()
{
	if (!GetEntity()->GetLevel()->GetScene()) FAIL;

	CString NodePath;
	GetEntity()->GetAttr<CString>(NodePath, CStrID("ScenePath"));
	CString NodeFile;
	GetEntity()->GetAttr<CString>(NodeFile, CStrID("SceneFile"));

	if (NodePath.IsEmpty() && NodeFile.IsValid())
		NodePath = GetEntity()->GetUID().CStr();
	
	if (NodePath.IsValid())
	{
		//???PERF: optimize duplicate search?
		Node = GetEntity()->GetLevel()->GetScene()->GetNode(NodePath.CStr(), false);
		ExistingNode = Node.IsValid();
		if (!ExistingNode) Node = GetEntity()->GetLevel()->GetScene()->GetNode(NodePath.CStr(), true);
		n_assert(Node.IsValid());

		if (NodeFile.IsValid()) n_assert(Scene::LoadNodesFromSCN("Scene:" + NodeFile + ".scn", Node));

		//???or do scene graph saving independently from entities? is this possible?
		//save level & all scene graph tfms, load level, apply tfms on nodes found?
		//64 (matrix) or 40 (SRT) bytes per node, no ChildTransforms attribute
		//Transform attr will be required because of many code abstracted from SceneNode is using Transform,
		//but this attribute won't be required to save. even if saved, it is not too much overhead
		//save scene graph as binary format (list or tree), either diff or overwrite, mb overwrite is better
		//after loading transforms UpdateTransform or smth must be fired, and no prop should use tfm before this happens

		if (ExistingNode)
			GetEntity()->SetAttr<matrix44>(CStrID("Transform"), Node->GetWorldMatrix());
		else
		{
			// Add children to the save-load list. All nodes externally attached won't be saved, which is desirable.
			ChildCache.BeginAdd();
			FillSaveLoadList(Node.GetUnsafe(), "");
			ChildCache.EndAdd();

			Node->SetWorldTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));

			// Load child local transforms
			Data::PDataArray ChildTfms = GetEntity()->GetAttr<Data::PDataArray>(CStrID("ChildTransforms"), NULL);
			if (ChildTfms.IsValid() && ChildTfms->GetCount())
			{
				for (int i = 0; i < ChildTfms->GetCount(); ++i)
				{
					Data::PParams ChildTfm = ChildTfms->Get<Data::PParams>(i);
					CStrID ChildID = ChildTfm->Get<CStrID>(CStrID("ID"));
					Scene::CSceneNode* pNode = GetChildNode(ChildID);
					if (pNode)
					{
						pNode->SetScale(ChildTfm->Get<vector3>(CStrID("S")));
						const vector4& Rot = ChildTfm->Get<vector4>(CStrID("R"));
						pNode->SetRotation(quaternion(Rot.x, Rot.y, Rot.z, Rot.w));
						pNode->SetPosition(ChildTfm->Get<vector3>(CStrID("T")));
					}
				}
			}
		}

		GetEntity()->FireEvent(CStrID("UpdateTransform"));
	}

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) EnableSI(*pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropSceneNode, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropSceneNode, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnLevelSaving, CPropSceneNode, OnLevelSaving);
	PROP_SUBSCRIBE_NEVENT(SetTransform, CPropSceneNode, OnSetTransform);
	PROP_SUBSCRIBE_PEVENT(AfterTransforms, CPropSceneNode, AfterTransforms);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropSceneNode, OnRenderDebugProc);
	OK;
}
//---------------------------------------------------------------------

void CPropSceneNode::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnLevelSaving);
	UNSUBSCRIBE_EVENT(SetTransform);
	UNSUBSCRIBE_EVENT(AfterTransforms);
	UNSUBSCRIBE_EVENT(OnRenderDebug);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	ChildCache.Clear();
	ChildrenToSave.Clear();

	if (Node.IsValid() && !ExistingNode)
	{
		Node->RemoveFromParent();
		Node = NULL;
	}
}
//---------------------------------------------------------------------

bool CPropSceneNode::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSceneNode::OnPropDeactivating(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSceneNode::OnLevelSaving(const Events::CEventBase& Event)
{
	// Need to recreate array because else we may rewrite initial level desc in the HRD cache
	Data::PDataArray ChildTfms = n_new(Data::CDataArray);
	GetEntity()->SetAttr<Data::PDataArray>(CStrID("ChildTransforms"), ChildTfms);

	Data::CData* pData = ChildTfms->Reserve(ChildrenToSave.GetCount());
	for (int i = 0; i < ChildrenToSave.GetCount(); ++i)
	{
		CStrID ChildID = ChildrenToSave[i];
		Scene::CSceneNode* pNode = GetChildNode(ChildID);
		if (!pNode) continue;
		if (!pNode->IsLocalTransformValid()) pNode->UpdateLocalFromWorld();
		Data::PParams ChildTfm = n_new(Data::CParams(4));
		ChildTfm->Set(CStrID("ID"), ChildID);
		ChildTfm->Set(CStrID("S"), pNode->GetScale());
		const quaternion& Rot = pNode->GetRotation();
		ChildTfm->Set(CStrID("R"), vector4(Rot.x, Rot.y, Rot.z, Rot.w));
		ChildTfm->Set(CStrID("T"), pNode->GetPosition());
		*pData = ChildTfm;
		++pData;
	}

	OK;
}
//---------------------------------------------------------------------

void CPropSceneNode::FillSaveLoadList(Scene::CSceneNode* pNode, const CString& Path)
{
	for (DWORD i = 0; i < pNode->GetChildCount(); ++i)
	{
		Scene::CSceneNode* pChild = pNode->GetChild(i);
		CString FullName = Path + pChild->GetName().CStr();
		CStrID FullID = CStrID(FullName.CStr());
		ChildCache.Add(FullID, pChild);
		ChildrenToSave.Add(FullID);
		FillSaveLoadList(pChild, FullName + ".");
	}
}
//---------------------------------------------------------------------

//???to node or some scene utils? recurse to subnodes?
void CPropSceneNode::GetAABB(CAABB& OutBox, DWORD TypeFlags) const
{
	if (!Node.IsValid() || !Node->GetAttrCount()) return;

	OutBox.BeginExtend();
	for (DWORD i = 0; i < Node->GetAttrCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node->GetAttr(i);
		if ((TypeFlags & AABB_Gfx) && Attr.IsA<Scene::CModel>())
		{
			CAABB AttrBox;
			((Scene::CRenderObject&)Attr).ValidateResources();
			((Scene::CModel&)Attr).GetGlobalAABB(AttrBox);
			OutBox.Extend(AttrBox);
		}
		else if ((TypeFlags & AABB_Phys) && Attr.IsA<Physics::CNodeAttrCollision>())
		{
			CAABB AttrBox;
			((Physics::CNodeAttrCollision&)Attr).GetGlobalAABB(AttrBox);
			OutBox.Extend(AttrBox);
		}
	}
	OutBox.EndExtend();
}
//---------------------------------------------------------------------

void CPropSceneNode::SetTransform(const matrix44& NewTfm)
{
	if (Node.IsValid()) Node->SetWorldTransform(NewTfm);
	GetEntity()->SetAttr<matrix44>(CStrID("Transform"), NewTfm);
	GetEntity()->FireEvent(CStrID("UpdateTransform"));
}
//---------------------------------------------------------------------

bool CPropSceneNode::OnSetTransform(const Events::CEventBase& Event)
{
	SetTransform(((Event::SetTransform&)Event).Transform);
	OK;
}
//---------------------------------------------------------------------

bool CPropSceneNode::AfterTransforms(const Events::CEventBase& Event)
{
	if (Node.IsValid() && Node->IsWorldMatrixChanged())
	{
		GetEntity()->SetAttr<matrix44>(CStrID("Transform"), Node->GetWorldMatrix());
		GetEntity()->FireEvent(CStrID("UpdateTransform"));
	}
	OK;
}
//---------------------------------------------------------------------

void CPropSceneNode::OnRenderDebug()
{
	DebugDraw->DrawCoordAxes(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));
}
//---------------------------------------------------------------------

}