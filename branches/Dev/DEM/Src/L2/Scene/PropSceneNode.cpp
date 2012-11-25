#include "PropSceneNode.h"

#include <Game/Entity.h>
#include <Scene/SceneServer.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DeclareAttr(Graphics);

	DefineString(ScenePath);
};

BEGIN_ATTRS_REGISTRATION(PropSceneNode)
	RegisterString(ScenePath, ReadOnly);
END_ATTRS_REGISTRATION

namespace Scene
{
	bool LoadNodesFromSCN(const nString& FileName, PSceneNode CurrRoot);
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
	Attrs.Append(Attr::Graphics); //???Attrs.Append(Attr::SceneResource);? - or completely on other props?
}
//---------------------------------------------------------------------

void CPropSceneNode::Activate()
{
	CPropTransformable::Activate();

	Node = SceneSrv->GetCurrentScene()->GetNode(GetEntity()->Get<nString>(Attr::ScenePath).Get(), true);
	n_assert(Node.isvalid());

	const nString& NodeRsrc = GetEntity()->Get<nString>(Attr::Graphics); //???rename attr?
	if (NodeRsrc.IsValid())
	{
		// Load scene resource from file
		//Scene::LoadNodesFromSCN();
	}

	if (Node->IsOwnedByScene())
		GetEntity()->Set<matrix44>(Attr::Transform, Node->GetWorldTransform());
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