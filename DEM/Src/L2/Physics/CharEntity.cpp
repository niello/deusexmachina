#include "CharEntity.h"

#include <Physics/PhysicsServerOld.h>
#include <Physics/RigidBody.h>
#include <Physics/Collision/Shape.h>
#include <Physics/PhysicsWorldOld.h>
#include <Physics/Joints/AMotor.h>
#include <Physics/Ragdoll.h>
#include <Game/GameServer.h>
#include <Game/GameLevel.h>

extern const matrix44 Rotate180(
	-1.f, 0.f,  0.f, 0.f,
	 0.f, 1.f,  0.f, 0.f,
	 0.f, 0.f, -1.f, 0.f,
	 0.f, 0.f,  0.f, 1.f);

namespace Physics
{
__ImplementClass(Physics::CCharEntity, 'PCEN', Physics::CEntity);

static const vector3 UpVector(0.0f, 1.0f, 0.0f);

CCharEntity::CCharEntity():
	Radius(0.3f),
	Height(1.75f),
	Hover(0.2f),
	DesiredAngularVel(0.f),
	MaxHorizAccel(30.f),
	GroundMtl(InvalidMaterial)
{
}
//---------------------------------------------------------------------

CCharEntity::~CCharEntity()
{
	n_assert(!DefaultComposite.IsValid());
}
//---------------------------------------------------------------------

// Create the abstract composite for the character entity when the entity
// is "alive" (not in its ragdoll state).
void CCharEntity::CreateDefaultComposite()
{
	DefaultComposite = CComposite::CreateInstance();

	PRigidBody BaseBody = CRigidBody::CreateInstance();
	BaseBody->Name = "CharEntityBody";

	float CapsuleLength = Height - 2.0f * Radius - Hover;
	matrix44 UpRight;
	UpRight.rotate_x(n_deg2rad(90.0f));
	UpRight.translate(vector3(0.0f, Hover + Radius + CapsuleLength * 0.5f, 0.0f));
	PShape pShape = (CShape*)PhysSrvOld->CreateCapsuleShape(UpRight,
		CMaterialTable::StringToMaterialType("Character"), Radius, CapsuleLength);
	BaseBody->BeginShapes(1);
	BaseBody->AddShape(pShape);
	BaseBody->EndShapes();
	DefaultComposite->BeginBodies(1);
	DefaultComposite->AddBody(BaseBody);
	DefaultComposite->EndBodies();

	Ptr<CAMotor> AMotor = CAMotor::CreateInstance();
	AMotor->SetBodies(BaseBody, NULL);
	AMotor->SetNumAxes(2);
	AMotor->AxisParams[0].Axis = vector3(1.0f, 0.0f, 0.0f);
	AMotor->AxisParams[1].Axis = vector3(0.0f, 0.0f, 1.0f);
	AMotor->AxisParams[0].FMax = 100000.0f;
	AMotor->AxisParams[1].FMax = 100000.0f;
	DefaultComposite->BeginJoints(1);
	DefaultComposite->AddJoint(AMotor);
	DefaultComposite->EndJoints();
}
//---------------------------------------------------------------------

// Create the optional ragdoll composite for the character entity.
void CCharEntity::CreateRagdollComposite()
{
	if (CompositeName.IsValid())
	{
		n_assert(CompositeName.IsValid());
		RagdollComposite = (Physics::CRagdoll*)PhysSrvOld->LoadCompositeFromPRM(CompositeName);
		n_assert(RagdollComposite->IsA(CRagdoll::RTTI));
		RagdollComposite->SetTransform(Transform);
		//RagdollComposite->SetCharacter(pNCharacter);
		RagdollComposite->Bind();
	}
}
//---------------------------------------------------------------------

void CCharEntity::Activate()
{
	n_assert(!DefaultComposite.IsValid());
	n_assert(!IsActive());
	
	CreateDefaultComposite();
	CreateRagdollComposite();
	
	SetComposite(DefaultComposite);
	EnableCollision();

	// We manually control body activity because AI may request very small speed
	// which is ignored, but we still want precise control over the body position.
	//???to all composite? (Composite->SetAutoFreezeParams(false))
	Composite->GetMasterBody()->SetAutoFreezeParams(false);

	Flags.Set(PHYS_ENT_ACTIVE);
}
//---------------------------------------------------------------------

void CCharEntity::Deactivate()
{
	n_assert(DefaultComposite);
	CEntity::Deactivate();
	DefaultComposite = NULL;
	RagdollComposite = NULL;
	//SetCharacter(NULL);
}
//---------------------------------------------------------------------

void CCharEntity::OnStepBefore()
{
	if (IsCollisionEnabled() && !IsRagdollActive())
	{
		Game::CSurfaceInfo SurfInfo;
		vector3 Pos = GetTransform().Translation();
		Pos.y += Height;

		// EPS
		float DistanceToGround;
		if (GameSrv->GetActiveLevel()->GetSurfaceInfoUnder(SurfInfo, Pos, Height + 0.1f, GetUID()))
		{
			DistanceToGround = Pos.y - Height - SurfInfo.WorldHeight;
			GroundMtl = SurfInfo.Material;
		}
		else
		{
			DistanceToGround = FLT_MAX;
			GroundMtl = InvalidMaterial;
		}

		CRigidBody* pMasterBody = Composite->GetMasterBody();
		bool BodyIsEnabled = IsEnabled();
		vector3 AngularVel = pMasterBody->GetAngularVelocity();
		vector3 LinearVel = pMasterBody->GetLinearVelocity();
		vector3 DesiredLVelChange = DesiredLinearVel - LinearVel;

 		if (DistanceToGround <= 0.f)
		{
			// No torques now, angular velocity changes by impulse immediately to desired value
			bool Actuated = DesiredAngularVel != AngularVel.y;
			if (Actuated)
			{
				if (!BodyIsEnabled) SetEnabled(true);
				pMasterBody->SetAngularVelocity(vector3(0.f, DesiredAngularVel, 0.f));
			}

			if (!DesiredLVelChange.isequal(vector3::Zero, 0.0001f))
			{
				if (!Actuated)
				{
					Actuated = true;
					SetEnabled(true);
				}

				// Vertical movement for non-flying actors is impulse (jump).
				// Since speed if already clamped to actor caps, we save vertical desired velocity as is.
				// Spring force pushes us from below the ground.
				
				//!!!!!!!!!!!!!!!
				//!!!calc correct impulse for the spring!
				//!!!!!!!!!!!!!!!
				
				float VerticalDesVelChange = DesiredLVelChange.y - (50.0f * DistanceToGround);

				float Mass = pMasterBody->GetMass();

				//!!!remove calcs for Y, it is zero (optimization)!
				dVector3 ODEForce;
				dWorldImpulseToForce(Level->GetODEWorldID(), dReal(Level->GetStepSize()),
					DesiredLVelChange.x * Mass, 0.f, DesiredLVelChange.z * Mass, ODEForce);
				float SqForceMagnitude = (float)dCalcVectorLengthSquare3(ODEForce);
				if (SqForceMagnitude > 0.f)
				{
					float MaxForceMagnitude = Mass * MaxHorizAccel;
					float SqMaxForceMagnitude = MaxForceMagnitude * MaxForceMagnitude;
					if (SqForceMagnitude > SqMaxForceMagnitude)
						dScaleVector3(ODEForce, MaxForceMagnitude / n_sqrt(SqForceMagnitude));
					dBodyAddForce(pMasterBody->GetODEBodyID(), ODEForce[0], ODEForce[1], ODEForce[2]);
				}
				
				if (VerticalDesVelChange != 0.f)
				{
					dWorldImpulseToForce(Level->GetODEWorldID(), dReal(Level->GetStepSize()),
						0.f, VerticalDesVelChange * Mass, 0.f, ODEForce);
					dBodyAddForce(pMasterBody->GetODEBodyID(), ODEForce[0], ODEForce[1], ODEForce[2]);
				}
			}

			if (BodyIsEnabled && !Actuated && DistanceToGround > -0.002f)
			{
				const float FreezeThreshold = 0.00001f; //???use TINY?

				bool AVelIsAlmostZero = n_fabs(AngularVel.y) < FreezeThreshold;
				bool LVelIsAlmostZero = n_fabs(LinearVel.x) * (float)Level->GetStepSize() < FreezeThreshold &&
										n_fabs(LinearVel.z) * (float)Level->GetStepSize() < FreezeThreshold;

				if (AVelIsAlmostZero)
					pMasterBody->SetAngularVelocity(vector3::Zero);

				if (LVelIsAlmostZero)
					pMasterBody->SetLinearVelocity(vector3::Zero);

				if (AVelIsAlmostZero && LVelIsAlmostZero) SetEnabled(false);
			}
		}
		//???!!!else (when falling) apply damping?!
	}
	// NOTE: do NOT call the parent class, we don't need any damping
}
//---------------------------------------------------------------------

void CCharEntity::ActivateRagdoll()
{
	n_assert(!IsRagdollActive());

	//!!!many unnecessary matrix copyings!
	if (RagdollComposite.IsValid())
	{
		// Get transform before detaching current composite
		matrix44 CurrTfm = GetTransform();

		// Switch current composite, this will reset the
		// composite's transform to the entity's initial transform
		DisableCollision();
		SetComposite(RagdollComposite);
		EnableCollision();

		// Fix transform (all the 180 voodoo is necessary because Nebula2's
		// characters are rotated by 180 degree around the Y axis
		matrix44 R180 = Rotate180;
		R180.mult_simple(CurrTfm);
		RagdollComposite->SetTransform(R180);

		// Sync physics joint angles with bind pose model
		RagdollComposite->ReadJoints();

		Flags.Set(RAGDOLL_ACTIVE);
	}
}
//---------------------------------------------------------------------

void CCharEntity::DeactivateRagdoll()
{
	n_assert(IsRagdollActive());

	Flags.Clear(RAGDOLL_ACTIVE);

	if (RagdollComposite.IsValid())
	{
		DisableCollision();
		SetComposite(DefaultComposite);
		EnableCollision();
	}
}
//---------------------------------------------------------------------

} // namespace Physics
