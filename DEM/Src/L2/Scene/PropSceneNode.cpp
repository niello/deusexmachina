#include "PropSceneNode.h"

#include <Game/Entity.h>
#include <Scene/SceneServer.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineString(ScenePath);
};

BEGIN_ATTRS_REGISTRATION(PropSceneNode)
	RegisterString(ScenePath, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropSceneNode, CPropTransformable);
ImplementFactory(Properties::CPropSceneNode);
RegisterProperty(CPropSceneNode);

void CPropSceneNode::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropTransformable::GetAttributes(Attrs);
	Attrs.Append(Attr::ScenePath);
	//???Attrs.Append(Attr::SceneResource);? - or completely on other props?
}
//---------------------------------------------------------------------

void CPropSceneNode::Activate()
{
	CPropTransformable::Activate();

	Node = SceneSrv->GetCurrentScene()->GetNode(GetEntity()->Get<nString>(Attr::ScenePath).Get(), true);
	n_assert(Node.isvalid());
	if (Node->IsOwnedByScene())
	{
		// get tfm from node and set as entity tfm
	}
	else Node->SetLocalTransform(GetEntity()->Get<matrix44>(Attr::Transform));
}
//---------------------------------------------------------------------

void CPropSceneNode::Deactivate()
{
	Node = NULL;
	CPropTransformable::Deactivate();
}
//---------------------------------------------------------------------

}