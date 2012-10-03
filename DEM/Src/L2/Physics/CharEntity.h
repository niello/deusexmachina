#pragma once
#ifndef __DEM_L2_PHYSICS_CHAR_ENTITY_H__
#define __DEM_L2_PHYSICS_CHAR_ENTITY_H__

#include "Entity.h"
#include <Physics/MaterialTable.h>
#include <Physics/FilterSet.h>
#include <Physics/AreaImpulse.h>
#include <Data/Flags.h>
#include <character/ncharacter2.h>

// Character physics controller. Supports default state in which actor is a capsule
// aligned along Up vector and hovered by a spring force, and ragdoll state.
// In default state controller also serves as an actuator of AI movement requests.

namespace Physics
{
using namespace Data;

typedef Ptr<class CComposite> PComposite;
typedef Ptr<class CRagdoll> PRagdoll;

class CCharEntity: public CEntity
{
	DeclareRTTI;
	DeclareFactory(CCharEntity);

protected:

	vector3			DesiredLinearVel;
	float			DesiredAngularVel;
	PComposite		DefaultComposite;
	PRagdoll		RagdollComposite;
	nCharacter2*	pNCharacter;
	CMaterialType	GroundMtl;
	PAreaImpulse	pRagdollImpulse;
	float			MaxHorizAccel;

	// See CEntity flags enum too
	enum
	{
		RAGDOLL_ACTIVE = 0x08
	};

	virtual void		CreateDefaultComposite();
	virtual void		CreateRagdollComposite();

public:

	float			Radius;
	float			Height;
	float			Hover;

	CCharEntity();
	virtual ~CCharEntity();

	virtual void		Activate();
	virtual void		Deactivate();
	virtual void		OnStepBefore();
	//virtual bool		OnCollide(CShape* pCollidee) { return CEntity::OnCollide(pCollidee) && !GroundExclSet.CheckShape(pCollidee); }
	void				ActivateRagdoll();
	void				DeactivateRagdoll();

	void				SetDesiredLinearVelocity(const vector3& Velocity) { DesiredLinearVel = Velocity; }
	const vector3&		GetDesiredLinearVelocity() const { return DesiredLinearVel; }
	void				SetDesiredAngularVelocity(float Velocity) { DesiredAngularVel = Velocity; }
	float				GetDesiredAngularVelocity() const { return DesiredAngularVel; }
	void				SetCharacter(nCharacter2* pNewChr);
	nCharacter2*		GetCharacter() const { return pNCharacter; }
	void				SetRagdollImpulse(CAreaImpulse* pImpulse) { pRagdollImpulse = pImpulse; }
	bool				IsRagdollActive() const { return Flags.Is(RAGDOLL_ACTIVE); }
	CMaterialType		GetGroundMaterial() const { return GroundMtl; }
};
//---------------------------------------------------------------------

RegisterFactory(CCharEntity);

typedef Ptr<CCharEntity> PCharEntity;

//!!!revisit!
inline void CCharEntity::SetCharacter(nCharacter2* pNewChr)
{
	if (pNCharacter != pNewChr)
	{
		if (pNCharacter) pNCharacter->Release();
		pNCharacter = pNewChr;
		if (pNCharacter) pNCharacter->AddRef();
	}
}
//---------------------------------------------------------------------

}

#endif
