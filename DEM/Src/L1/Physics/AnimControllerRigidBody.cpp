#include "AnimControllerRigidBody.h"

namespace Anim
{

void CAnimControllerRigidBody::SetBody(btRigidBody* pRigidBody)
{
	n_assert(pRigidBody);
	pRB = pRigidBody;
	Channels.Set(Chnl_Translation | Chnl_Rotation);
}
//---------------------------------------------------------------------

void CAnimControllerRigidBody::Clear()
{
	Channels.ClearAll();
	pRB = NULL;
}
//---------------------------------------------------------------------

bool CAnimControllerRigidBody::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!pRB) FAIL;

	OK;
}
//---------------------------------------------------------------------

}
