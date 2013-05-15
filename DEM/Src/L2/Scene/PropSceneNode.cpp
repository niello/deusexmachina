#include "PropSceneNode.h"

#include <Game/Entity.h>
#include <Scene/SceneServer.h>
#include <Scene/Model.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineString(ScenePath);
	DefineString(SceneFile);
};

BEGIN_ATTRS_REGISTRATION(PropSceneNode)
	RegisterString(ScenePath, ReadOnly);
	RegisterString(SceneFile, ReadOnly);
END_ATTRS_REGISTRATION

namespace Scene
{
	bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode, bool PreloadResources = true);
}

namespace Properties
{
__ImplementClassNoFactory(Properties::CPropSceneNode, CPropTransformable);
__ImplementClass(Properties::CPropSceneNode);
__ImplementPropertyStorage(CPropSceneNode, 256); //!!!remove if : public CPropTransformable
RegisterProperty(CPropSceneNode);

void CPropSceneNode::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	//CPropTransformable::GetAttributes(Attrs);
	CProperty::GetAttributes(Attrs);
	Attrs.Append(Attr::ScenePath);
	Attrs.Append(Attr::SceneFile);
}
//---------------------------------------------------------------------

void CPropSceneNode::Activate()
{
	//CPropTransformable::Activate();
	CProperty::Activate();

	nString NodePath = GetEntity()->Get<nString>(Attr::ScenePath);
	const nString& NodeRsrc = GetEntity()->Get<nString>(Attr::SceneFile);

	if (NodePath.IsEmpty() && NodeRsrc.IsValid())
		NodePath = GetEntity()->GetUID().CStr();
	
	if (NodePath.IsValid())
	{
		//???optimize duplicate search?
		Node = SceneSrv->GetCurrentScene()->GetNode(NodePath.CStr(), false);
		ExistingNode = Node.IsValid();
		if (!ExistingNode) Node = SceneSrv->GetCurrentScene()->GetNode(NodePath.CStr(), true);
		n_assert(Node.IsValid());

		if (NodeRsrc.IsValid()) n_assert(Scene::LoadNodesFromSCN("scene:" + NodeRsrc + ".scn", Node));

		if (ExistingNode)
			GetEntity()->Set<matrix44>(Attr::Transform, Node->GetWorldMatrix());
		else Node->SetLocalTransform(GetEntity()->Get<matrix44>(Attr::Transform)); //???set local? or set global & then calc local?
	}
}
//---------------------------------------------------------------------

void CPropSceneNode::Deactivate()
{
	if (Node.IsValid() && !ExistingNode)
	{
		Node->RemoveFromParent();
		Node = NULL;
	}
	//CPropTransformable::Deactivate();
	CProperty::Deactivate();
}
//---------------------------------------------------------------------

void CPropSceneNode::GetAABB(bbox3& OutBox) const
{
	if (!Node.IsValid() || !Node->GetAttrCount()) return;

	OutBox.begin_extend();
	for (DWORD i = 0; i < Node->GetAttrCount(); ++i)
	{
		Scene::CSceneNodeAttr& Attr = *Node->GetAttr(i);
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

}