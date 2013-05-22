#pragma once
#ifndef __DEM_L2_PHYSICS_RBODY_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_RBODY_H__

#include <Physics/PhysicsServerOld.h>
#include <Physics/MaterialTable.h>
#include <mathlib/matrix.h>
#include <mathlib/vector.h>
#include <mathlib/plane.h>
#include <util/nfixedarray.h>
#define dSINGLE
#include <ode/ode.h>

// CRigidBody is the internal base class for all types of rigid bodies.
// RigidBodies can be connected by Joints to form a hierarchy.

namespace Physics
{
typedef Ptr<class CShape> PShape;
class CComposite;
class CEntity;

class CRigidBody: public Core::CRefCounted
{
	__DeclareClass(CRigidBody);

public:

    typedef unsigned int Id;

private:

	friend class CShape;

	static Id UIDCounter;

	matrix44				InitialTfm;
	matrix44				InvInitialTfm;
	matrix44				Transform;
	Id						UID;
	nFixedArray<nString>	LinkNames;
	CEntity*				pEntity;
	dBodyID					ODEBodyID;
	dMass					Mass;			// the Mass structure of the rigid body
	int						CurrShapeIndex;
	nFixedArray<PShape>		Shapes;
	float					AngularDamping;
	float					LinearDamping;

	void ApplyDamping();

public:

	// Link types (for linking to graphical representation)
	enum ELinkType
	{
		ModelNode = 0,
		ShadowNode,
		JointNode,
		NumLinkTypes
	};

	nString		Name;
	CComposite*	Composite;
	int			LinkIndex;
	uint		Stamp;
	bool		CollideConnected; // Enable/disable collision for bodies connected by a joint (if both true).

	CRigidBody();
	virtual ~CRigidBody();

	void			Attach(dWorldID WorldID, dSpaceID SpaceID, const matrix44& Tfm);
	void			Detach();
	bool			SetAutoFreezeParams(bool AutoFreeze);
	void			OnStepBefore();
	void			OnStepAfter();
	void			OnFrameBefore();
	void			OnFrameAfter();
	void			RenderDebug();
	void			ApplyImpulseAtPos(const vector3& Impulse, const vector3& Pos);
	void			Reset();

	void			BeginShapes(int Count);
	void			AddShape(CShape* pShape);
	void			EndShapes();
	int				GetNumShapes() const { return Shapes.GetCount(); }
	CShape*			GetShapeAt(int Idx) const { return Shapes[Idx]; }

	bool			IsAttached() const { return ODEBodyID != NULL; }
	Id				GetUID() const { return UID; }
	void			SetEntity(CEntity* pEnt);
	CEntity*		GetEntity() const { return pEntity; }
	void			SetEnabled(bool Enable);
	bool			IsEnabled() const { return dBodyIsEnabled(ODEBodyID) > 0; }
	void			SetLinkName(ELinkType Type, const nString& Name) { LinkNames[Type] = Name; }
	const nString&	GetLinkName(ELinkType Type) const { return LinkNames[Type]; }
	bool			IsLinkValid(ELinkType Type) const { return LinkNames[Type].IsValid(); }
	void			GetAABB(bbox3& AABB) const;
	void			SetTransform(const matrix44& Tfm);
	const matrix44& GetTransform() const { return Transform; }
	void			SetInitialTransform(const matrix44& Tfm);
	const matrix44&	GetInitialTransform() const { return InitialTfm; }
	const matrix44&	GetInvInitialTransform() const { return InvInitialTfm; }
	void			SetLinearVelocity(const vector3& Velocity);
	vector3			GetLinearVelocity() const;
	void			SetAngularVelocity(const vector3& Velocity);
	vector3			GetAngularVelocity() const;
	vector3			GlobalToLocalPoint(const vector3& Point) const;
	vector3			LocalToGlobalPoint(const vector3& Point) const;
	void			GetLocalForce(vector3& OutForce) const;
	void			GetLocalTorque(vector3& OutTorque) const;
	int				GetNumCollisions() const;
	bool			IsHorizontalCollided() const;
	void			SetAngularDamping(float Damping);
	float			GetAngularDamping() const { return AngularDamping; }
	void			SetLinearDamping(float Damping);
	float			GetLinearDamping() const { return LinearDamping; }
	void			SetEnableGravity(bool Enable) { dBodySetGravityMode(ODEBodyID, Enable ? 1 : 0); }
	bool			GetEnableGravity() const { return dBodyGetGravityMode(ODEBodyID) != 0; }
	float			GetMass() const { return Mass.mass; }
	dBodyID			GetODEBodyID() const { return ODEBodyID; }
};
//---------------------------------------------------------------------

typedef Ptr<CRigidBody> PRigidBody;

// Enable/disable the rigid body. Bodies which have reached a resting position
// should be disabled. Bodies will Enable themselves as soon as they are
// touched by another body.
inline void CRigidBody::SetEnabled(bool Enable)
{
	if (Enable) dBodyEnable(ODEBodyID);
	else dBodyDisable(ODEBodyID);
}
//---------------------------------------------------------------------

inline void CRigidBody::Reset()
{
	dBodySetForce(ODEBodyID, 0.0f, 0.0f, 0.0f);
	dBodySetTorque(ODEBodyID, 0.0f, 0.0f, 0.0f);
}
//---------------------------------------------------------------------

inline void CRigidBody::SetInitialTransform(const matrix44& Tfm)
{
	InitialTfm = Tfm;
	InvInitialTfm = Tfm;
	Tfm.invert_simple(InvInitialTfm);
}
//---------------------------------------------------------------------

inline void CRigidBody::SetAngularDamping(float Damping)
{
	n_assert(Damping >= 0.0f && Damping <= 1.0f);
	AngularDamping = Damping;
}
//---------------------------------------------------------------------

inline void CRigidBody::SetLinearDamping(float Damping)
{
	n_assert(Damping >= 0.0f && Damping <= 1.0f);
	LinearDamping = Damping;
}
//---------------------------------------------------------------------

inline void CRigidBody::SetLinearVelocity(const vector3& Velocity)
{
	dBodySetLinearVel(ODEBodyID, Velocity.x, Velocity.y, Velocity.z);
}
//---------------------------------------------------------------------

inline vector3 CRigidBody::GetLinearVelocity() const
{
	vector3 Velocity;
	CPhysicsServerOld::OdeToVector3(*(dVector3*)dBodyGetLinearVel(ODEBodyID), Velocity);
	return Velocity;
}
//---------------------------------------------------------------------

inline void CRigidBody::SetAngularVelocity(const vector3& Velocity)
{
	dBodySetAngularVel(ODEBodyID, Velocity.x, Velocity.y, Velocity.z);
}
//---------------------------------------------------------------------

inline vector3 CRigidBody::GetAngularVelocity() const
{
	vector3 Velocity;
	CPhysicsServerOld::OdeToVector3(*(dVector3*)dBodyGetAngularVel(ODEBodyID), Velocity);
	return Velocity;
}
//---------------------------------------------------------------------

inline void CRigidBody::GetLocalForce(vector3& OutForce) const
{
	CPhysicsServerOld::OdeToVector3(*(dVector3*)dBodyGetForce(ODEBodyID), OutForce);
}
//---------------------------------------------------------------------

inline void CRigidBody::GetLocalTorque(vector3& OutTorque) const
{
	CPhysicsServerOld::OdeToVector3(*(dVector3*)dBodyGetTorque(ODEBodyID), OutTorque);
}
//---------------------------------------------------------------------

// Transforms a point in global space into the local coordinate system of the body.
inline vector3 CRigidBody::GlobalToLocalPoint(const vector3& Point) const
{
	dVector3 Result;
	dBodyGetPosRelPoint(ODEBodyID, Point.x, Point.y, Point.z, Result);
	return vector3(Result[0], Result[1], Result[2]);
}
//---------------------------------------------------------------------

// Transforms a point in body-local space into global space.
inline vector3 CRigidBody::LocalToGlobalPoint(const vector3& Point) const
{
	dVector3 Result;
	dBodyGetRelPointPos(ODEBodyID, Point.x, Point.y, Point.z, Result);
	return vector3(Result[0], Result[1], Result[2]);
}
//---------------------------------------------------------------------

}

#endif
