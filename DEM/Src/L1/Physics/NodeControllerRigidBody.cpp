#include "NodeControllerRigidBody.h"

namespace Physics
{
//
//void CNodeControllerRigidBody::SetBody(btRigidBody* pRigidBody)
//{
//	n_assert(pRigidBody);
//	pRB = pRigidBody;
//	Channels.Set(Chnl_Translation | Chnl_Rotation);
//}
////---------------------------------------------------------------------

void CNodeControllerRigidBody::Clear()
{
	Channels.ClearAll();
//	pRB = NULL;
}
//---------------------------------------------------------------------

bool CNodeControllerRigidBody::ApplyTo(Math::CTransformSRT& DestTfm)
{
//	if (!pRB) FAIL;

	OK;
}
//---------------------------------------------------------------------

}
