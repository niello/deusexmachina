#pragma once
#ifndef __DEM_L2_PHYSICS_RAY_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_RAY_H__

#include <Core/RefCounted.h>
#include <Physics/ContactPoint.h>
#include <Physics/FilterSet.h>
#define dSINGLE
#include <ode/ode.h>

// CRay objects are used to perform ray checks on the physics world representation.

namespace Physics
{

class CRay: public Core::CRefCounted
{
	__DeclareClass(CRay);

private:

	enum { MaxContacts = 16 };

	static nArray<CContactPoint>* Contacts; // not owned!

	vector3		Origin;
	vector3		Direction;
	vector3		DirNorm;
	CFilterSet	ExcludeFilter;
	dGeomID		ODERayId;

	// ODE collision callback function
	static void OdeRayCallback(void* data, dGeomID o1, dGeomID o2);

public:

	CRay();
	virtual ~CRay();

	virtual int			DoRayCheckAllContacts(const matrix44& Tfm, nArray<CContactPoint>& OutContacts);

	void				SetOrigin(const vector3& Orig) { Origin = Orig; }
	const vector3&		GetOrigin() const { return Origin; }
	void				SetDirection(const vector3& v);
	const vector3&		GetDirection() const { return Direction; }
	const vector3&		GetNormDirection() const { return DirNorm; }
	void				SetExcludeFilterSet(const CFilterSet& Filter) { ExcludeFilter = Filter; }
	const CFilterSet&	GetExcludeFilterSet() const { return ExcludeFilter; }
	dGeomID				GetGeomId() const { return ODERayId; }
};
//---------------------------------------------------------------------

inline void CRay::SetDirection(const vector3& Dir)
{
	Direction = Dir;
	DirNorm = Dir;
	DirNorm.norm();
}
//---------------------------------------------------------------------

}

#endif
