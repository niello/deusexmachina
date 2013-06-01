#include "NodeControllerRigidBody.h"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//for channel flags, move flags to Scene!
#include <Animation/Anim.h>

namespace Physics
{

void CNodeControllerRigidBody::SetBody(CRigidBody& RigidBody)
{
	Body = &RigidBody;
	Channels.Set(Anim::Chnl_Translation | Anim::Chnl_Rotation);
}
//---------------------------------------------------------------------

void CNodeControllerRigidBody::Clear()
{
	Channels.ClearAll();
	Body = NULL;
}
//---------------------------------------------------------------------

bool CNodeControllerRigidBody::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!Body.IsValid()) FAIL;

	//always update the first time after activation
	//then look at body motion state flag "changed this frame"

	Body->GetTransform(DestTfm.Translation, DestTfm.Rotation);

	OK;
}
//---------------------------------------------------------------------

}
