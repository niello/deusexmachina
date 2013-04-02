#include "PropSceneNode.h"

#include <Game/Entity.h>
#include <Scene/SceneServer.h>
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
ImplementRTTI(Properties::CPropSceneNode, CPropTransformable);
ImplementFactory(Properties::CPropSceneNode);
ImplementPropertyStorage(CPropSceneNode, 256); //!!!remove if : public CPropTransformable
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
		NodePath = GetEntity()->GetUniqueID().CStr();
	
	if (NodePath.IsValid())
	{
		//???optimize duplicate search?
		Node = SceneSrv->GetCurrentScene()->GetNode(NodePath.Get(), false);
		ExistingNode = Node.isvalid();
		if (!ExistingNode) Node = SceneSrv->GetCurrentScene()->GetNode(NodePath.Get(), true);
		n_assert(Node.isvalid());

		if (NodeRsrc.IsValid()) n_assert(Scene::LoadNodesFromSCN("scene:" + NodeRsrc + ".scn", Node));

		if (ExistingNode)
			GetEntity()->Set<matrix44>(Attr::Transform, Node->GetWorldMatrix());
		else Node->SetLocalTransform(GetEntity()->Get<matrix44>(Attr::Transform)); //???set local? or set global & then calc local?
	}
}
//---------------------------------------------------------------------

void CPropSceneNode::Deactivate()
{
	if (Node.isvalid() && !ExistingNode)
	{
		Node->RemoveFromParent();
		Node = NULL;
	}
	//CPropTransformable::Deactivate();
	CProperty::Deactivate();
}
//---------------------------------------------------------------------

}