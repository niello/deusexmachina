#include "NodeControllerRigidBody.h"

namespace Physics
{

void CNodeControllerRigidBody::SetBody(CRigidBody& RigidBody)
{
	Body = &RigidBody;
	Channels.Set(Scene::Tfm_Translation | Scene::Tfm_Rotation);
	Body->SetTransformChanged(true); // To enforce first update
}
//---------------------------------------------------------------------

void CNodeControllerRigidBody::Clear()
{
	Channels.ClearAll();
	Body = NULL;
}
//---------------------------------------------------------------------

//!!!Transform checks and sets restrict usage of this controller to 1 body - 1 controller - 1 node
//It is like HACK to control "is body transform changed" flag from body-based controller, which should
//be read-only, but I can't find a better way for now.
bool CNodeControllerRigidBody::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (Body.IsNullPtr() || !Body->IsTransformChanged()) FAIL;

	Body->GetTransform(DestTfm.Translation, DestTfm.Rotation);
	Body->SetTransformChanged(false);

	OK;
}
//---------------------------------------------------------------------

}
