#include "PropSceneNode.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scripting/PropScriptable.h>
#include <Scene/Events/SetTransform.h>
#include <Frame/Model.h>
#include <Physics/NodeAttrCollision.h>
#include <Data/DataArray.h>
#include <Debug/DebugDraw.h>
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
	if (!GetEntity()->GetLevel()->GetSceneRoot()) FAIL;

	CString NodePath;
	GetEntity()->GetAttr<CString>(NodePath, CStrID("ScenePath"));
	CString NodeFile;
	GetEntity()->GetAttr<CString>(NodeFile, CStrID("SceneFile"));

	if (NodePath.IsEmpty() && NodeFile.IsValid())
		NodePath = GetEntity()->GetUID().CStr();
	
	if (NodePath.IsValid())
	{
		const char* pUnresolved;
		Node = GetEntity()->GetLevel()->GetSceneRoot()->FindDeepestChild(NodePath.CStr(), pUnresolved);
		ExistingNode = !pUnresolved;
		if (pUnresolved) Node = Node->CreateChildChain(pUnresolved);
		n_assert(Node.IsValidPtr());

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
			FillSaveLoadList(Node.GetUnsafe(), CString::Empty);
			ChildCache.EndAdd();

			Node->SetWorldTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));

			// Load child local transforms
			Data::PDataArray ChildTfms = GetEntity()->GetAttr<Data::PDataArray>(CStrID("ChildTransforms"), NULL);
			if (ChildTfms.IsValidPtr() && ChildTfms->GetCount())
			{
				for (UPTR i = 0; i < ChildTfms->GetCount(); ++i)
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

	//???for each other active property of this entity call InitDependency(Prop)?
	//switchcase of prop type and dependent code, centralized!
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

	//???for each other active property of this entity call TermDependency(Prop)?
	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	ChildCache.Clear();
	ChildrenToSave.Clear();

	if (Node.IsValidPtr() && !ExistingNode)
	{
		Node->Remove();
		Node = NULL;
	}
}
//---------------------------------------------------------------------

bool CPropSceneNode::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	//???call InitDependency(Prop)?
	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSceneNode::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	//???call TermDependency(Prop)?
	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSceneNode::OnLevelSaving(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	// Need to recreate array because else we may rewrite initial level desc in the HRD cache
	Data::PDataArray ChildTfms = n_new(Data::CDataArray);
	GetEntity()->SetAttr<Data::PDataArray>(CStrID("ChildTransforms"), ChildTfms);

	Data::CData* pData = ChildTfms->Reserve(ChildrenToSave.GetCount());
	for (UPTR i = 0; i < ChildrenToSave.GetCount(); ++i)
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

void CPropSceneNode::FillSaveLoadList(Scene::CSceneNode* pNode, const char* pPath)
{
	for (UPTR i = 0; i < pNode->GetChildCount(); ++i)
	{
		Scene::CSceneNode* pChild = pNode->GetChild(i);
		CString FullName(pPath);
		FullName += pChild->GetName().CStr();
		CStrID FullID = CStrID(FullName.CStr());
		ChildCache.Add(FullID, pChild);
		ChildrenToSave.Add(FullID);
		FillSaveLoadList(pChild, FullName + ".");
	}
}
//---------------------------------------------------------------------

//???to some scene utils? recurse to subnodes?
void CPropSceneNode::GetAABB(CAABB& OutBox, UPTR TypeFlags) const
{
	if (Node.IsNullPtr() || !Node->GetAttributeCount()) return;

	OutBox.BeginExtend();
	for (UPTR i = 0; i < Node->GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node->GetAttribute(i);
		if ((TypeFlags & AABB_Gfx) && Attr.IsA<Frame::CModel>())
		{
			CAABB AttrBox;
			((Frame::CRenderObject&)Attr).ValidateResources();
			((Frame::CModel&)Attr).GetGlobalAABB(AttrBox);
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
	if (Node.IsValidPtr()) Node->SetWorldTransform(NewTfm);
	GetEntity()->SetAttr<matrix44>(CStrID("Transform"), NewTfm);
	GetEntity()->FireEvent(CStrID("UpdateTransform"));
}
//---------------------------------------------------------------------

bool CPropSceneNode::OnSetTransform(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	SetTransform(((Event::SetTransform&)Event).Transform);
	OK;
}
//---------------------------------------------------------------------

bool CPropSceneNode::AfterTransforms(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (Node.IsValidPtr() && Node->IsWorldMatrixChanged())
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