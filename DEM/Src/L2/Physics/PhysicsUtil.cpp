#include "PhysicsUtil.h"
#include <Physics/PhysicsServer.h>

namespace Physics
{

// Do a normal ray check and return the closest intersection.
bool CPhysicsUtil::RayTest(const vector3& From, const vector3& To, CContactPoint& OutContact,
							const CFilterSet* ExcludeSet)
{
	//PhysicsSrv->RayTest(From, To - From, ExcludeSet);

	float MinSqDist = 10000000000.0f;
	const nArray<CContactPoint>& Contacts = PhysicsSrv->GetContactPoints();
	int Idx = INVALID_INDEX;
	for (int i = 0; i < PhysicsSrv->GetContactPoints().GetCount(); i++)
	{
		float SqDist = vector3(Contacts[i].Position - From).lensquared();
		if (SqDist < MinSqDist)
		{
			MinSqDist = SqDist;
			Idx = i;
		}
	}

	if (Idx != INVALID_INDEX)
	{
		OutContact = Contacts[Idx];
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

// Do a ray bundle stabbing check (4 spaced out rays) into the world and
// return whether a HasIntersection happened, and if yes where.
// If no intersection, returns distance between From & To
bool CPhysicsUtil::RayBundleCheck(const vector3& From, const vector3& To, const vector3& Up,
								  const vector3& Left, float Radius,
								  float& OutContactDist, const CFilterSet* ExcludeSet)
{
	vector3 StabDir = To - From;
	OutContactDist = StabDir.lensquared();
	bool HasIntersection = false;

	for (int i = 0; i < 4; i++)
	{
		vector3 RayPos;
		switch (i)
		{
			case 0: RayPos = From + Up * Radius; break;
			case 1: RayPos = From - Up * Radius; break;
			case 2: RayPos = From + Left * Radius; break;
			case 3: RayPos = From - Left * Radius; break;
			default: break;
		}

		//PhysicsSrv->RayTest(RayPos, StabDir, ExcludeSet);

		const nArray<CContactPoint>& Contacts = PhysicsSrv->GetContactPoints();
		for (int j = 0; j < PhysicsSrv->GetContactPoints().GetCount(); j++)
		{
			float SqDist = vector3(Contacts[j].Position - From).lensquared();
			if (SqDist < OutContactDist)
			{
				OutContactDist = SqDist;
				HasIntersection = true;
			}
		}
	}

	OutContactDist = (float)n_sqrt(OutContactDist);
	return HasIntersection;
}
//---------------------------------------------------------------------

} // namespace Physics
