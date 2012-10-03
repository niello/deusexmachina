#include "ContactPoint.h"

#include <Physics/PhysicsServer.h>
#include <Physics/Entity.h>
#include <Physics/Composite.h>

namespace Physics
{

CEntity* CContactPoint::GetEntity() const
{
	return PhysicsSrv->FindEntityByUniqueID(EntityID);
}
//---------------------------------------------------------------------

CRigidBody* CContactPoint::GetRigidBody() const
{
	CEntity* pEnt = GetEntity();
	return (pEnt) ? pEnt->GetComposite()->FindBodyByUniqueID(RigidBodyID) : NULL;
}
//---------------------------------------------------------------------

}
