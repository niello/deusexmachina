#include "ContactPoint.h"

#include <Physics/PhysicsServerOld.h>
#include <Physics/Entity.h>
#include <Physics/Composite.h>

namespace Physics
{

CEntity* CContactPoint::GetEntity() const
{
	return PhysSrvOld->FindEntityByUniqueID(EntityID);
}
//---------------------------------------------------------------------

CRigidBodyOld* CContactPoint::GetRigidBody() const
{
	CEntity* pEnt = GetEntity();
	return (pEnt) ? pEnt->GetComposite()->FindBodyByUniqueID(RigidBodyID) : NULL;
}
//---------------------------------------------------------------------

}
