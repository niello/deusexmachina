#include "PropSceneNode.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/SceneServer.h>
#include <Scene/Model.h>
#include <Scene/Events/SetTransform.h>
#include <Render/DebugDraw.h>

namespace Scene
{
	bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode, bool PreloadResources = true);
}

namespace Prop
{
__ImplementClass(Prop::CPropSceneNode, 'PSCN', Game::CProperty);
__ImplementPropertyStorage(CPropSceneNode);

IMPL_EVENT_HANDLER_VIRTUAL(OnRenderDebug, CPropSceneNode, OnRenderDebug)

bool CPropSceneNode::InternalActivate()
{
	nString NodePath;
	GetEntity()->GetAttr<nString>(NodePath, CStrID("ScenePath"));
	nString NodeFile;
	GetEntity()->GetAttr<nString>(NodeFile, CStrID("SceneFile"));

	if (NodePath.IsEmpty() && NodeFile.IsValid())
		NodePath = GetEntity()->GetUID().CStr();
	
	if (NodePath.IsValid())
	{
		//???optimize duplicate search?
		Node = GetEntity()->GetLevel().GetScene()->GetNode(NodePath.CStr(), false);
		ExistingNode = Node.IsValid();
		if (!ExistingNode) Node = GetEntity()->GetLevel().GetScene()->GetNode(NodePath.CStr(), true);
		n_assert(Node.IsValid());

		if (NodeFile.IsValid()) n_assert(Scene::LoadNodesFromSCN("scene:" + NodeFile + ".scn", Node));

		if (ExistingNode)
			GetEntity()->SetAttr<matrix44>(CStrID("Transform"), Node->GetWorldMatrix());
		else Node->SetWorldTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));
		GetEntity()->FireEvent(CStrID("UpdateTransform"));
	}

	PROP_SUBSCRIBE_NEVENT(SetTransform, CPropSceneNode, OnSetTransform);
	PROP_SUBSCRIBE_PEVENT(OnWorldTfmsUpdated, CPropSceneNode, OnWorldTfmsUpdated);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropSceneNode, OnRenderDebugProc);
	OK;
}
//---------------------------------------------------------------------

void CPropSceneNode::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(SetTransform);
	UNSUBSCRIBE_EVENT(OnWorldTfmsUpdated);
	UNSUBSCRIBE_EVENT(OnRenderDebug);

	if (Node.IsValid() && !ExistingNode)
	{
		Node->RemoveFromParent();
		Node = NULL;
	}
}
//---------------------------------------------------------------------

//???to node or some scene utils?
void CPropSceneNode::GetAABB(bbox3& OutBox) const
{
	if (!Node.IsValid() || !Node->GetAttrCount()) return;

	OutBox.begin_extend();
	for (DWORD i = 0; i < Node->GetAttrCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node->GetAttr(i);
		if (Attr.IsA<Scene::CModel>())
		{
			bbox3 AttrBox;
			((Scene::CModel&)Attr).GetGlobalAABB(AttrBox);
			OutBox.extend(AttrBox);
		}
	}
	OutBox.end_extend();
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

bool CPropSceneNode::OnWorldTfmsUpdated(const Events::CEventBase& Event)
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