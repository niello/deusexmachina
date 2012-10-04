#include "RigidBody.h"

#include <Physics/Collision/Shape.h>
#include <Physics/Composite.h>
#include <Physics/Level.h>

namespace Physics
{
ImplementRTTI(Physics::CRigidBody, Core::CRefCounted);
ImplementFactory(Physics::CRigidBody);

uint CRigidBody::UIDCounter = 1;

CRigidBody::CRigidBody():
	Composite(NULL),
	CurrShapeIndex(0),
	pEntity(NULL),
	LinkNames(NumLinkTypes),
	LinkIndex(-1),
	CollideConnected(false),
	AngularDamping(0.01f),
	LinearDamping(0.005f),
	ODEBodyID(NULL),
	Stamp(0)
{
	UID = UIDCounter++;
}
//---------------------------------------------------------------------

CRigidBody::~CRigidBody()
{
	if (IsAttached()) Detach();
}
//---------------------------------------------------------------------

// A rigid body's shape can be described by any number of geometry shapes which are attached to the rigid body.
void CRigidBody::BeginShapes(int Count)
{
	n_assert(!Shapes.Size());
	Shapes.SetSize(Count);
	CurrShapeIndex = 0;
}
//---------------------------------------------------------------------

void CRigidBody::AddShape(CShape* pShape)
{
	n_assert(pShape);
	pShape->SetRigidBody(this);
	pShape->SetEntity(pEntity);
	Shapes[CurrShapeIndex++] = pShape;
}
//---------------------------------------------------------------------

void CRigidBody::EndShapes()
{
	n_assert(Shapes.Size() == CurrShapeIndex);
}
//---------------------------------------------------------------------

// Attach the rigid body to the world and initialize its position.
// This will create an ODE rigid body object and create all associated shapes.
void CRigidBody::Attach(dWorldID WorldID, dSpaceID SpaceID, const matrix44& Tfm)
{
	n_assert(!IsAttached());

	ODEBodyID = dBodyCreate(WorldID);
	dBodySetData(ODEBodyID, this);

	dMassSetZero(&Mass);
	for (int i = 0; i < Shapes.Size(); i++)
	{
		CShape* pShape = Shapes[i];
		if (pShape->Attach(SpaceID)) dMassAdd(&Mass, &pShape->ODEMass);
		else n_error("CRigidBody::Attach(): Failed to attach pShape!");
	}

	dMassTranslate(&Mass, -Mass.c[0], -Mass.c[1], -Mass.c[2]);
	dBodySetMass(ODEBodyID, &Mass);
	SetTransform(Tfm);
}
//---------------------------------------------------------------------

void CRigidBody::Detach()
{
	n_assert(IsAttached());
	for (int i = 0; i < Shapes.Size(); i++) Shapes[i]->Detach();
	dBodyDestroy(ODEBodyID);
	ODEBodyID = NULL;
}
//---------------------------------------------------------------------

bool CRigidBody::SetAutoFreezeParams(bool AutoFreeze)
{
	if (ODEBodyID)
	{
		dBodySetAutoDisableFlag(ODEBodyID, AutoFreeze != 0);
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

void CRigidBody::OnFrameBefore()
{
	for (int i = 0; i < Shapes.Size(); i++)
	{
		CShape* pShape = Shapes[i];
		pShape->SetNumCollisions(0);
		pShape->SetHorizontalCollided(false);
	}
}
//---------------------------------------------------------------------

void CRigidBody::OnFrameAfter()
{
}
//---------------------------------------------------------------------

// Apply an Impulse vector at a position in the global coordinate frame.
void CRigidBody::ApplyImpulseAtPos(const vector3& Impulse, const vector3& Pos)
{
	n_assert(IsAttached());
	dWorldID ODEWorldID = PhysicsSrv->GetLevel()->GetODEWorldID();
	SetEnabled(true);
	dVector3 ODEForce;
	dWorldImpulseToForce(ODEWorldID, dReal(PhysicsSrv->GetLevel()->GetStepSize()), Impulse.x, Impulse.y, Impulse.z, ODEForce);
	dBodyAddForceAtPos(ODEBodyID, ODEForce[0], ODEForce[1], ODEForce[2], Pos.x, Pos.y, Pos.z);
}
//---------------------------------------------------------------------

// Apply linear and angular damping.
void CRigidBody::ApplyDamping()
{
	n_assert(IsEnabled());

	if (AngularDamping > 0.0f)
	{
		const dReal* ODEVector = dBodyGetAngularVel(ODEBodyID);
		vector3 Velocity(ODEVector[0], ODEVector[1], ODEVector[2]);
		float Damp = n_saturate(1.0f - AngularDamping);
		Velocity *= Damp;

		// clamp at some upper value
		float SqLen = Velocity.lensquared();
		if (SqLen > 30.f * 30.f) Velocity *= 30.f / (float)n_sqrt(SqLen);

		dBodySetAngularVel(ODEBodyID, Velocity.x, Velocity.y, Velocity.z);
	}

	if (LinearDamping > 0.0f)
	{
		const dReal* ODEVector = dBodyGetLinearVel(ODEBodyID);
		vector3 Velocity(ODEVector[0], ODEVector[1], ODEVector[2]);
		float Damp = n_saturate(1.0f - LinearDamping);
		Velocity *= Damp;

		// clamp at some upper value
		float SqLen = Velocity.lensquared();
		if (SqLen > 50.f * 50.f) Velocity *= 50.f / (float)n_sqrt(SqLen);
		
		dBodySetLinearVel(ODEBodyID, Velocity.x, Velocity.y, Velocity.z);
	}
}
//---------------------------------------------------------------------

void CRigidBody::OnStepBefore()
{
    CLevel* pLevel = PhysicsSrv->GetLevel();
    n_assert(pLevel);
    
	dSpaceID DynamicSpace = pLevel->GetODEDynamicSpaceID();
    dSpaceID StaticSpace = pLevel->GetODEStaticSpaceID();
	
	if (IsEnabled())
	{
		// move to dynamic collide space if we have become freshly enabled
		for (int i = 0; i < Shapes.Size(); i++)
		{
			CShape* pShape = Shapes[i];
			if (DynamicSpace != pShape->GetSpaceId())
			{
				pShape->RemoveFromSpace();
				pShape->AttachToSpace(DynamicSpace);
			}
		}

		ApplyDamping();
	}
    else
    {
		// move to static collide space if we have become freshly disabled
		for (int i = 0; i < Shapes.Size(); i++)
		{
			CShape* pShape = Shapes[i];
			if (StaticSpace != pShape->GetSpaceId())
			{
				pShape->RemoveFromSpace();
				pShape->AttachToSpace(StaticSpace);
			}
		}
	}
}
//---------------------------------------------------------------------

void CRigidBody::OnStepAfter()
{
	n_assert(IsAttached());
	const dReal* Pos = dBodyGetPosition(ODEBodyID);
	const dReal* Rot = dBodyGetRotation(ODEBodyID);
	vector3 PosVector;
	CPhysicsServer::OdeToMatrix44(*(dMatrix3*)Rot, Transform);
	CPhysicsServer::OdeToVector3(*(dVector3*)Pos, PosVector);
	Transform.M41 = PosVector.x;
	Transform.M42 = PosVector.y;
	Transform.M43 = PosVector.z;
}
//---------------------------------------------------------------------

int CRigidBody::GetNumCollisions() const
{
	int Result = 0;
	for (int i = 0; i < Shapes.Size(); i++)
		Result += Shapes[i]->GetNumCollisions();
	return Result;
}
//---------------------------------------------------------------------

bool CRigidBody::IsHorizontalCollided() const
{
	for (int i = 0; i < Shapes.Size(); i++)
		if (Shapes[i]->IsHorizontalCollided()) OK;
	FAIL;
}
//---------------------------------------------------------------------

void CRigidBody::GetAABB(bbox3& AABB) const
{
	if (Shapes.Size() > 0)
	{
		Shapes[0]->GetAABB(AABB);
		for (int i = 0; i < Shapes.Size(); i++)
		{
			bbox3 NextAABB;
			Shapes[i]->GetAABB(NextAABB);
			//???transform?
			AABB.extend(NextAABB);
		}
		//???transform?
	}
	else
	{
		AABB.vmin.x = 
		AABB.vmin.y = 
		AABB.vmin.z = 
		AABB.vmax.x = 
		AABB.vmax.y = 
		AABB.vmax.z = 0.f;
	}
}
//---------------------------------------------------------------------

// Set the body's current transformation in global space.
// Only translation and rotation is allowed (no scale or shear).
void CRigidBody::SetTransform(const matrix44& Tfm)
{
	Transform = Tfm;

	if (IsAttached())
	{
		dMatrix3 ODERotation;
		CPhysicsServer::Matrix44ToOde(Tfm, ODERotation);
		dBodySetRotation(ODEBodyID, ODERotation);
		const vector3& Pos = Transform.pos_component();
		dBodySetPosition(ODEBodyID, Pos.x, Pos.y, Pos.z);
	}
}
//---------------------------------------------------------------------

void CRigidBody::RenderDebug()
{
	if (IsAttached())
		for (int i = 0; i < Shapes.Size(); i++)
			Shapes[i]->RenderDebug(Transform);
}
//---------------------------------------------------------------------

void CRigidBody::SetEntity(CEntity* pEnt)
{
	n_assert(pEnt);
	pEntity = pEnt;
	for (int i = 0; i < Shapes.Size(); i++)
		Shapes[i]->SetEntity(pEnt);
}
//---------------------------------------------------------------------

} // namespace Physics
