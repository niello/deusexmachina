#pragma once
#ifndef __DEM_L2_PHYSICS_SERVER_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_SERVER_H__

#include <Core/RefCounted.h>
#include <Data/Singleton.h>
#include <Physics/Entity.h>
#include <Physics/ContactPoint.h>
#include <Physics/FilterSet.h>
#include <util/nkeyarray.h>

// Physics server. Description TO BE ADDED!

namespace Physics
{
typedef Ptr<class CLevel> PLevel;
typedef Ptr<class CShape> PShape;
class CBoxShape;
class CSphereShape;
class CCapsuleShape;
class CMeshShape;
class CHeightfieldShape;
class CRay;
class CAreaImpulse;

#define PhysicsSrv Physics::CPhysicsServer::Instance()

class CPhysicsServer: public Core::CRefCounted
{
	__DeclareClass(CPhysicsServer);
	__DeclareSingleton(CPhysicsServer);

protected:
	
	typedef nArray<CContactPoint> CContacts;

	static uint				UniqueStamp;

	nKeyArray<CEntity*>		Entities;
	CContacts				Contacts;
	PLevel					CurrLevel;

	bool					isOpen;

	void RegisterEntity(CEntity* pEnt) { n_assert(pEnt); Entities.Add(pEnt->GetUniqueID(), pEnt); }
	void UnregisterEntity(CEntity* pEnt) { n_assert(pEnt); Entities.Rem(pEnt->GetUniqueID()); }

	friend class CLevel;
	friend class CRigidBody;
	friend class TerrainEntity;
	friend class MouseGripper;
	friend class CEntity;

public:

	CPhysicsServer();
	virtual ~CPhysicsServer();

	virtual bool	Open();
	virtual void	Close();
	bool			IsOpen() const { return isOpen; }
	virtual void	Trigger();

	CBoxShape*			CreateBoxShape(const matrix44& TF, CMaterialType MatType, const vector3& Size) const;
	CSphereShape*		CreateSphereShape(const matrix44& TF, CMaterialType MatType, float Radius) const;
	CCapsuleShape*		CreateCapsuleShape(const matrix44& TF, CMaterialType MatType, float Radius, float Length) const;
	CMeshShape*			CreateMeshShape(const matrix44& TF, CMaterialType MatType, const nString& FileName) const;
	CHeightfieldShape*	CreateHeightfieldShape(const matrix44& TF, CMaterialType MatType, const nString& FileName) const;
	CRay*				CreateRay(const vector3& Origin, const vector3& Dir) const;
	
	CComposite*			LoadCompositeFromPRM(const nString& Name) const;

	bool			ApplyImpulseAlongRay(const vector3& Pos, const vector3& Dir, float Impulse,
										 const CFilterSet* ExcludeSet = NULL);

	CEntity*		FindEntityByUniqueID(DWORD uniqueId) const;
	int				GetEntitiesInShape(PShape Shape, const CFilterSet& ExcludeSet,
									   nArray<PEntity>& Result);
	int				GetEntitiesInSphere(const vector3& Pos, float Radius, const CFilterSet& ExcludeSet,
										nArray<PEntity>& Result);
	int				GetEntitiesInBox(const vector3& Scale, const matrix44& TF, const CFilterSet& ExcludeSet,
									 nArray<PEntity>& Result);

	virtual void	RenderDebug();

	static void		Matrix44ToOde(const matrix44& from, dMatrix3& to);
	static void		OdeToMatrix44(const dMatrix3& from, matrix44& to);
	static void		Vector3ToOde(const vector3& from, dVector3& to);
	static void		OdeToVector3(const dVector3& from, vector3& to);

	bool					RayCheck(const vector3& Pos, const vector3& Dir, const CFilterSet* ExcludeSet = NULL);
	const CContacts&		GetContactPoints() const { return Contacts; }
	const CContactPoint*	GetClosestContactAlongRay(const vector3& Pos, const vector3& Dir, const CFilterSet* ExcludeSet = NULL);

	static uint		GetUniqueStamp() { return ++UniqueStamp; }

	void			SetLevel(CLevel* pLevel);
	CLevel*			GetLevel() const { return CurrLevel.get_unsafe(); }
	//void			SetPointOfInterest(const vector3& NewPOI);
	//const vector3&	GetPointOfInterest() const { return CurrLevel->GetPointOfInterest(); }
};

RegisterFactory(CPhysicsServer);

// Converts the rotational part of a matrix44 To ODE. This includes
// a handedness conversion (transpose), since Mangalore is right handed, ODE left handed(?)
inline void CPhysicsServer::Matrix44ToOde(const matrix44& From, dMatrix3& To)
{
	To[0] = From.M11; To[1] = From.M21; To[2] = From.M31; To[3] = 0.0f;
	To[4] = From.M12; To[5] = From.M22; To[6] = From.M32; To[7] = 0.0f;
	To[8] = From.M13; To[9] = From.M23; To[10] = From.M33; To[11] = 0.0f;
}
//---------------------------------------------------------------------

// Convert a dMatrix3 To the rotational part of a matrix44. Includes handedness conversion.
inline void CPhysicsServer::OdeToMatrix44(const dMatrix3& From, matrix44& To)
{
	To.M11 = From[0]; To.M12 = From[4]; To.M13 = From[8];
	To.M21 = From[1]; To.M22 = From[5]; To.M23 = From[9];
	To.M31 = From[2]; To.M32 = From[6]; To.M33 = From[10];
}
//---------------------------------------------------------------------

inline void CPhysicsServer::Vector3ToOde(const vector3& From, dVector3& To)
{
	To[0] = From.x; To[1] = From.y; To[2] = From.z; To[3] = 1.0f;
}
//---------------------------------------------------------------------

inline void CPhysicsServer::OdeToVector3(const dVector3& From, vector3& To)
{
	To.set(From[0], From[1], From[2]);
}
//---------------------------------------------------------------------

inline CEntity* CPhysicsServer::FindEntityByUniqueID(DWORD UID) const
{
	CEntity* pEnt = NULL;
	if (UID) Entities.Find(UID, pEnt);
	return pEnt;
}
//---------------------------------------------------------------------

// Set the current point of interest for the physics subsystem. This can
// be for instance the position of the game entity which has the input focus.
// Only the area around this point of interest should be simulated.
//inline void CPhysicsServer::SetPointOfInterest(const vector3& NewPOI)
//{
//	n_assert(CurrLevel);
//	CurrLevel->SetPointOfInterest(NewPOI);
//}
//---------------------------------------------------------------------

}

#endif
