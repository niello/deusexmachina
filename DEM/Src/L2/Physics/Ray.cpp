#include "Ray.h"

#include <Physics/Level.h>
#include <Physics/Collision/Shape.h>
#include <Physics/Composite.h>

namespace Physics
{
ImplementRTTI(Physics::CRay, Core::CRefCounted);
ImplementFactory(Physics::CRay);

nArray<CContactPoint>* CRay::Contacts = NULL;

CRay::CRay(): Direction(0.0f, 1.0f, 0.0f)
{
	ODERayId = dCreateRay(0, 1.0f);
}
//---------------------------------------------------------------------

CRay::~CRay()
{
	n_assert(!Contacts);
	dGeomDestroy(ODERayId);
}
//---------------------------------------------------------------------

void CRay::OdeRayCallback(void* data, dGeomID o1, dGeomID o2)
{
	n_assert(data);
	n_assert(o1 != o2);
	n_assert(CRay::Contacts);

	CRay* pSelf = (CRay*)data;
	n_assert(pSelf->IsInstanceOf(CRay::RTTI));

	// handle sub-space
	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2(o1, o2, data, &OdeRayCallback);
		return;
	}

	// check for exclusion
	CShape* pOtherShape;
	if (o1 == pSelf->GetGeomId()) pOtherShape = CShape::GetShapeFromGeom(o2);
	else if (o2 == pSelf->GetGeomId()) pOtherShape = CShape::GetShapeFromGeom(o1);
	else pOtherShape = NULL; //???is allowed?

	if (pSelf->GetExcludeFilterSet().CheckShape(pOtherShape)) return;

	dContactGeom ODEContact[MaxContacts];
	int CollCount = (o1 == pSelf->GetGeomId()) ?
		dCollide(o2, o1, MaxContacts, ODEContact, sizeof(dContactGeom)) :
		dCollide(o1, o2, MaxContacts, ODEContact, sizeof(dContactGeom));

	CContactPoint ContactPt;
	for (int i = 0; i < CollCount; i++)
	{
		// FIXME: hmm, ODEContact[x].geom.pos[] doesn't seem to be correct with mesh
		// shapes which are not at the origin. Computing the intersection pos from
		// the stabbing depth and the ray's original vector
		ContactPt.Position = pSelf->GetOrigin() + pSelf->GetNormDirection() * ODEContact[i].depth;
		ContactPt.UpVector.set(ODEContact[i].normal[0], ODEContact[i].normal[1], ODEContact[i].normal[2]);
		ContactPt.Depth = ODEContact[i].depth;
		CEntity* pOtherEntity = pOtherShape->GetEntity();
		ContactPt.EntityID = pOtherEntity ? pOtherEntity->GetUniqueID() : 0;
		CRigidBody* pOtherRB = pOtherShape->GetRigidBody();
		ContactPt.RigidBodyID = pOtherRB ? pOtherRB->GetUniqueID() : 0;
		ContactPt.Material = pOtherShape->GetMaterialType();
		CRay::Contacts->Append(ContactPt);
	}
}
//---------------------------------------------------------------------

// Do a complete ray check, and append(!) all OutContacts to the provided
// vector3 array. Returns number of intersections encountered.
// - 24-Nov-03     floh    dGeomRaySet() was expecting a normalized direction
int CRay::DoRayCheckAllContacts(const matrix44& Tfm, nArray<CContactPoint>& OutContacts)
{
	if (!PhysicsSrv->GetLevel()) return 0;

	int InitialContactCount = OutContacts.Size();

	vector3 GlobalOrig = Tfm * Origin;
	vector3 GlobalDir  = (Tfm * (Origin + Direction)) - GlobalOrig;
	float RayLength = GlobalDir.len();
	GlobalDir.norm();

	dGeomRaySet(ODERayId, GlobalOrig.x, GlobalOrig.y, GlobalOrig.z, GlobalDir.x, GlobalDir.y, GlobalDir.z);
	dGeomRaySetLength(ODERayId, RayLength);

	dSpaceID ODESpaceID = PhysicsSrv->GetLevel()->GetODECommonSpaceID();
	CRay::Contacts = &OutContacts;
	dSpaceCollide2((dGeomID)ODESpaceID, ODERayId, this, &OdeRayCallback);
	CRay::Contacts = NULL;
	return OutContacts.Size() - InitialContactCount;
}
//---------------------------------------------------------------------

} // namespace Physics
