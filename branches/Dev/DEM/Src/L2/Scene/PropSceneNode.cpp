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
RegisterProperty(CPropSceneNode);

void CPropSceneNode::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropTransformable::GetAttributes(Attrs);
	Attrs.Append(Attr::ScenePath);
	Attrs.Append(Attr::SceneFile);
}
//---------------------------------------------------------------------

void CPropSceneNode::Activate()
{
	CPropTransformable::Activate();

	//???optimize duplicate search?
	LPCSTR pPath = GetEntity()->Get<nString>(Attr::ScenePath).Get();
	Node = SceneSrv->GetCurrentScene()->GetNode(pPath, false);
	bool NodeExists = Node.isvalid();
	if (!NodeExists) Node = SceneSrv->GetCurrentScene()->GetNode(pPath, true);
	n_assert(Node.isvalid());

	const nString& NodeRsrc = GetEntity()->Get<nString>(Attr::SceneFile);
	if (NodeRsrc.IsValid()) n_assert(Scene::LoadNodesFromSCN("scene:" + NodeRsrc + ".scn", Node));

	if (NodeExists)
		GetEntity()->Set<matrix44>(Attr::Transform, Node->GetWorldMatrix());
	else Node->SetLocalTransform(GetEntity()->Get<matrix44>(Attr::Transform)); //???set local? or set global & then calc local?
}
//---------------------------------------------------------------------

void CPropSceneNode::Deactivate()
{
	Node = NULL;
	CPropTransformable::Deactivate();
}
//---------------------------------------------------------------------

}