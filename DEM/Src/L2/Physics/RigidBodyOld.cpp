#include "RigidBodyOld.h"

#include <Physics/Collision/Shape.h>
#include <Physics/Composite.h>
#include <Physics/PhysicsWorldOld.h>

namespace Physics
{
__ImplementClass(Physics::CRigidBodyOld, 'RGBD', Core::CRefCounted);

uint CRigidBodyOld::UIDCounter = 1;

CRigidBodyOld::CRigidBodyOld():
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

CRigidBodyOld::~CRigidBodyOld()
{
	if (IsAttached()) Detach();
}
//---------------------------------------------------------------------

// A rigid body's shape can be described by any number of geometry shapes which are attached to the rigid body.
void CRigidBodyOld::BeginShapes(int Count)
{
	n_assert(!Shapes.GetCount());
	Shapes.SetSize(Count);
	CurrShapeIndex = 0;
}
//---------------------------------------------------------------------

void CRigidBodyOld::AddShape(CShape* pShape)
{
	n_assert(pShape);
	pShape->SetRigidBody(this);
	pShape->SetEntity(pEntity);
	Shapes[CurrShapeIndex++] = pShape;
}
//---------------------------------------------------------------------

void CRigidBodyOld::EndShapes()
{
	n_assert(Shapes.GetCount() == CurrShapeIndex);
}
//---------------------------------------------------------------------

// Attach the rigid body to the world and initialize its position.
// This will create an ODE rigid body object and create all associated shapes.
void CRigidBodyOld::Attach(dWorldID WorldID, dSpaceID SpaceID, const matrix44& Tfm)
{
	n_assert(!IsAttached());

	ODEBodyID = dBodyCreate(WorldID);
	dBodySetData(ODEBodyID, this);

	dMassSetZero(&Mass);
	for (int i = 0; i < Shapes.GetCount(); i++)
	{
		CShape* pShape = Shapes[i];
		if (pShape->Attach(SpaceID)) dMassAdd(&Mass, &pShape->ODEMass);
		else n_error("CRigidBodyOld::Attach(): Failed to attach pShape!");
	}

	dMassTranslate(&Mass, -Mass.c[0], -Mass.c[1], -Mass.c[2]);
	dBodySetMass(ODEBodyID, &Mass);
	SetTransform(Tfm);
}
//---------------------------------------------------------------------

void CRigidBodyOld::Detach()
{
	n_assert(IsAttached());
	for (int i = 0; i < Shapes.GetCount(); i++) Shapes[i]->Detach();
	dBodyDestroy(ODEBodyID);
	ODEBodyID = NULL;
}
//---------------------------------------------------------------------

bool CRigidBodyOld::SetAutoFreezeParams(bool AutoFreeze)
{
	if (ODEBodyID)
	{
		dBodySetAutoDisableFlag(ODEBodyID, AutoFreeze != 0);
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

void CRigidBodyOld::OnFrameBefore()
{
	for (int i = 0; i < Shapes.GetCount(); i++)
	{
		CShape* pShape = Shapes[i];
		pShape->SetNumCollisions(0);
		pShape->SetHorizontalCollided(false);
	}
}
//---------------------------------------------------------------------

void CRigidBodyOld::OnFrameAfter()
{
}
//---------------------------------------------------------------------

// Apply an Impulse vector at a position in the global coordinate frame.
void CRigidBodyOld::ApplyImpulseAtPos(const vector3& Impulse, const vector3& Pos)
{
	//n_assert(IsAttached());
	//dWorldID ODEWorldID = PhysSrvOld->GetLevel()->GetODEWorldID();
	//SetEnabled(true);
	//dVector3 ODEForce;
	//dWorldImpulseToForce(ODEWorldID, dReal(PhysSrvOld->GetLevel()->GetStepSize()), Impulse.x, Impulse.y, Impulse.z, ODEForce);
	//dBodyAddForceAtPos(ODEBodyID, ODEForce[0], ODEForce[1], ODEForce[2], Pos.x, Pos.y, Pos.z);
}
//---------------------------------------------------------------------

// Apply linear and angular damping.
void CRigidBodyOld::ApplyDamping()
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

void CRigidBodyOld::OnStepBefore()
{
	/*
    CPhysWorldOld* pLevel = PhysSrvOld->GetLevel();
    n_assert(pLevel);
    
	dSpaceID DynamicSpace = pLevel->GetODEDynamicSpaceID();
    dSpaceID StaticSpace = pLevel->GetODEStaticSpaceID();
	
	if (IsEnabled())
	{
		// move to dynamic collide space if we have become freshly enabled
		for (int i = 0; i < Shapes.GetCount(); i++)
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
		for (int i = 0; i < Shapes.GetCount(); i++)
		{
			CShape* pShape = Shapes[i];
			if (StaticSpace != pShape->GetSpaceId())
			{
				pShape->RemoveFromSpace();
				pShape->AttachToSpace(StaticSpace);
			}
		}
	}
	*/
}
//---------------------------------------------------------------------

void CRigidBodyOld::OnStepAfter()
{
	n_assert(IsAttached());
	CPhysicsServerOld::OdeToMatrix44(*(dMatrix3*)dBodyGetRotation(ODEBodyID), Transform);
	CPhysicsServerOld::OdeToVector3(*(dVector3*)dBodyGetPosition(ODEBodyID), Transform.Translation());
}
//---------------------------------------------------------------------

int CRigidBodyOld::GetNumCollisions() const
{
	int Result = 0;
	for (int i = 0; i < Shapes.GetCount(); i++)
		Result += Shapes[i]->GetNumCollisions();
	return Result;
}
//---------------------------------------------------------------------

bool CRigidBodyOld::IsHorizontalCollided() const
{
	for (int i = 0; i < Shapes.GetCount(); i++)
		if (Shapes[i]->IsHorizontalCollided()) OK;
	FAIL;
}
//---------------------------------------------------------------------

void CRigidBodyOld::GetAABB(bbox3& AABB) const
{
	if (Shapes.GetCount() > 0)
	{
		Shapes[0]->GetAABB(AABB);
		for (int i = 0; i < Shapes.GetCount(); i++)
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
void CRigidBodyOld::SetTransform(const matrix44& Tfm)
{
	Transform = Tfm;

	if (IsAttached())
	{
		dMatrix3 ODERotation;
		CPhysicsServerOld::Matrix44ToOde(Tfm, ODERotation);
		dBodySetRotation(ODEBodyID, ODERotation);
		const vector3& Pos = Transform.Translation();
		dBodySetPosition(ODEBodyID, Pos.x, Pos.y, Pos.z);
	}
}
//---------------------------------------------------------------------

void CRigidBodyOld::RenderDebug()
{
	if (IsAttached())
		for (int i = 0; i < Shapes.GetCount(); i++)
			Shapes[i]->RenderDebug(Transform);
}
//---------------------------------------------------------------------

void CRigidBodyOld::SetEntity(CEntity* pEnt)
{
	n_assert(pEnt);
	pEntity = pEnt;
	for (int i = 0; i < Shapes.GetCount(); i++)
		Shapes[i]->SetEntity(pEnt);
}
//---------------------------------------------------------------------

} // namespace Physics
