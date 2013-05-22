#include "ExplosionAreaImpulse.h"

#include <Physics/PhysicsServer.h>
#include <Physics/Collision/SphereShape.h>
#include <Physics/RigidBody.h>
#include <Physics/PhysicsWorldOld.h>

namespace Physics
{
__ImplementClass(Physics::CExplosionAreaImpulse, 'EXAI', Physics::CAreaImpulse);

nArray<CContactPoint> CExplosionAreaImpulse::CollideContacts(256, 512);

void CExplosionAreaImpulse::Apply()
{
	// create a sphere shape and collide it against the world
	matrix44 Tfm;
	Tfm.translate(Position);
	Ptr<CSphereShape> pShape =
		PhysicsSrv->CreateSphereShape(Tfm,
									  CMaterialTable::StringToMaterialType("Wood"), //???Invalid mtl?
									  Radius);
	//pShape->Attach(PhysicsSrv->GetLevel()->GetODEDynamicSpaceID());
	CFilterSet ExcludeSet;
	pShape->Collide(ExcludeSet, CollideContacts);
	pShape->Detach();

	// apply Impulse to rigid bodies
	uint Stamp = CPhysicsServer::GetUniqueStamp();
	for (int i = 0; i < CollideContacts.GetCount(); i++)
	{
		CRigidBody* pBody = CollideContacts[i].GetRigidBody();
		if (pBody && pBody->Stamp != Stamp)
		{
			pBody->Stamp = Stamp;
			HandleRigidBody(pBody, CollideContacts[i].Position);
		}
	}
}
//---------------------------------------------------------------------

// Applies Impulse on single rigid body. Does line of sight test on the center of the
// rigid body (FIXME: check all corners of the bounding box??).
bool CExplosionAreaImpulse::HandleRigidBody(CRigidBody* pBody, const vector3& Position)
{
	// Do line of sight check to position of body
	//CFilterSet ExcludeSet;
	//ExcludeSet.AddRigidBodyId(pBody->GetUID());
	vector3 Dir = Position - Position;
	//if (0 == PhysicsSrv->GetClosestRayContact(Position, Dir, ExcludeSet))
	//{
		// free line of sight, apply Impulse
		float Distance = Dir.len();
		Dir *= (1.f / Distance);

		// scale Impulse by distance
		vector3 Impulse = Dir * Impulse * (1.0f - n_saturate(Distance / Radius));

		pBody->ApplyImpulseAtPos(Impulse, Position);
		OK;
	//}
	//else
	//{
		//// no free line of sight
		//FAIL;
	//}
}
//---------------------------------------------------------------------

} // namespace Physics
