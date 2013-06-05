#include "Entity.h"

#include <Physics/Composite.h>
#include <Physics/PhysicsWorldOld.h>
#include <Physics/Joints/AMotor.h>
#include <Game/GameServer.h>

namespace Physics
{
__ImplementClass(Physics::CEntity, 'PENT', Core::CRefCounted);

DWORD CEntity::UIDCounter = 1;

CEntity::CEntity():
	Level(NULL),
	UserData(CStrID::Empty),
	Stamp(0),
	Radius(0.3f),
	Height(1.75f),
	Hover(0.2f),
	DesiredAngularVel(0.f),
	MaxHorizAccel(30.f),
	GroundMtl(InvalidMaterial)
{
	UID = UIDCounter++;
	CollidedShapes.Flags.Set(Array_DoubleGrowSize);
	PhysSrvOld->RegisterEntity(this);
}
//---------------------------------------------------------------------

CEntity::~CEntity()
{
	n_assert(!Composite.IsValid());
	n_assert(!Level);
	if (IsActive()) Deactivate();
	PhysSrvOld->UnregisterEntity(this);
}
//---------------------------------------------------------------------

void CEntity::Activate()
{
	n_assert(!Composite.IsValid());
	n_assert(!IsActive());
	
	Composite = CComposite::CreateInstance();

	PRigidBodyOld BaseBody = CRigidBodyOld::CreateInstance();
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
	Composite->BeginBodies(1);
	Composite->AddBody(BaseBody);
	Composite->EndBodies();

	Ptr<CAMotor> AMotor = CAMotor::CreateInstance();
	AMotor->SetBodies(BaseBody, NULL);
	AMotor->SetNumAxes(2);
	AMotor->AxisParams[0].Axis = vector3(1.0f, 0.0f, 0.0f);
	AMotor->AxisParams[1].Axis = vector3(0.0f, 0.0f, 1.0f);
	AMotor->AxisParams[0].FMax = 100000.0f;
	AMotor->AxisParams[1].FMax = 100000.0f;
	Composite->BeginJoints(1);
	Composite->AddJoint(AMotor);
	Composite->EndJoints();
	
	SetComposite(Composite);
	EnableCollision();

	// We manually control body activity because AI may request very small speed
	// which is ignored, but we still want precise control over the body position.
	//???to all composite? (Composite->SetAutoFreezeParams(false))
	Composite->GetMasterBody()->SetAutoFreezeParams(false);

	Flags.Set(PHYS_ENT_ACTIVE);
}
//---------------------------------------------------------------------

void CEntity::Deactivate()
{
	n_assert(Composite.IsValid());
	n_assert(IsActive());
	DisableCollision();
	Composite = NULL;
	Flags.Clear(PHYS_ENT_ACTIVE);
}
//---------------------------------------------------------------------

void CEntity::OnAttachedToLevel(CPhysWorldOld* pLevel)
{
	n_assert(pLevel && !Level);
	Level = pLevel;
	SetEnabled(false);
}
//---------------------------------------------------------------------

void CEntity::OnRemovedFromLevel()
{
	n_assert(Level);
	Level = NULL;
}
//---------------------------------------------------------------------

// Returns is collision valid
bool CEntity::OnCollide(CShape* pCollidee)
{
	if (!CollidedShapes.Find(pCollidee))
		CollidedShapes.Append(pCollidee);
	OK;
}
//---------------------------------------------------------------------

// Set the current transformation in world space. This method should
// only be called once at initialization time, since the main job
// of a physics object is to COMPUTE the transformation for a game entity.
void CEntity::SetTransform(const matrix44& Tfm)
{
	if (Composite.IsValid()) Composite->SetTransform(Tfm);
	Transform = Tfm;
}
//---------------------------------------------------------------------

//???INLINE?
// The transformation is updated during Physics::Server::Trigger().
matrix44 CEntity::GetTransform() const
{
	return (Composite.IsValid()) ? Composite->GetTransform() : Transform;
}
//---------------------------------------------------------------------

//???INLINE?
// Return true if the transformation has changed during the frame.
bool CEntity::HasTransformChanged() const
{
	return Composite.IsValid() && Composite->HasTransformChanged();
}
//---------------------------------------------------------------------

//???INLINE?
vector3 CEntity::GetVelocity() const
{
	return (Composite.IsValid() && Composite->GetMasterBody()) ?
		Composite->GetMasterBody()->GetLinearVelocity() :
		vector3::Zero;
}
//---------------------------------------------------------------------

//???INLINE?
void CEntity::OnStepBefore()
{
	if (IsCollisionEnabled())
	{
		Game::CSurfaceInfo SurfInfo;
		vector3 Pos = GetTransform().Translation();
		Pos.y += Height;

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

		CRigidBodyOld* pMasterBody = Composite->GetMasterBody();
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

void CEntity::OnStepAfter()
{
	if (Composite.IsValid()) Composite->OnStepAfter();
	if (IsLocked() && IsEnabled())
	{
		SetTransform(LockedTfm);
		SetEnabled(false);
	}
}
//---------------------------------------------------------------------

// This method is invoked before a physics frame starts (consisting of several physics steps).
void CEntity::OnFrameBefore()
{
	CollidedShapes.Clear();
	if (Composite.IsValid()) Composite->OnFrameBefore();
}
//---------------------------------------------------------------------

void CEntity::OnFrameAfter()
{
	if (Composite.IsValid()) Composite->OnFrameAfter();
}
//---------------------------------------------------------------------

//???INLINE?
void CEntity::Reset()
{
	if (Composite.IsValid()) Composite->Reset();
}
//---------------------------------------------------------------------

//???INLINE?
int CEntity::GetNumCollisions() const
{
	return (Composite.IsValid()) ? Composite->GetNumCollisions() : 0;
}
//---------------------------------------------------------------------

//???INLINE?
bool CEntity::IsHorizontalCollided() const
{
	return Composite.IsValid() && Composite->IsHorizontalCollided();
}
//---------------------------------------------------------------------

//???INLINE?
// A disabled entity will enable itself automatically on contact with other enabled entities.
void CEntity::SetEnabled(bool Enabled)
{
	if (Composite.IsValid()) Composite->SetEnabled(Enabled);
}
//---------------------------------------------------------------------

bool CEntity::IsEnabled() const
{
	return Composite.IsValid() && Composite->IsEnabled();
}
//---------------------------------------------------------------------

// Lock the entity. A locked entity acts like a disabled entity,
// but will never re-enable itself on contact with another entity.
void CEntity::Lock()
{
	n_assert(!IsLocked());
	Flags.Set(PHYS_ENT_LOCKED);
	LockedTfm = GetTransform(); //!!!get matrix memory here (pool/alloc)!
	SetEnabled(false);
}
//---------------------------------------------------------------------

// Unlock the entity. This will reset the entity (set velocity and forces
// to 0), and place it on the position where it was when the entity was
// locked. Note that the entity will NOT be enabled. This will happen
// automatically when necessary (for instance on contact with another
// active entity).
void CEntity::Unlock()
{
	n_assert(IsLocked());
	Flags.Clear(PHYS_ENT_LOCKED);
	SetTransform(LockedTfm);
	Reset();
}
//---------------------------------------------------------------------

void CEntity::SetComposite(CComposite* pNew)
{
	n_assert(pNew);
	Composite = pNew;
	Composite->SetEntity(this);
	Composite->SetTransform(Transform);
	if (IsLocked()) Composite->SetEnabled(false);
}
//---------------------------------------------------------------------

//???INLINE?
void CEntity::RenderDebug()
{
	if (Composite.IsValid()) Composite->RenderDebug();
}
//---------------------------------------------------------------------

//???rename?
void CEntity::EnableCollision()
{
	if (!IsCollisionEnabled() && Composite.IsValid())
	{
		Composite->Attach(Level->GetODEWorldID(), Level->GetODEDynamicSpaceID(), Level->GetODEStaticSpaceID());
		Flags.Set(PHYS_ENT_COLLISION_ENABLED);
	}
}
//---------------------------------------------------------------------

//???rename?
void CEntity::DisableCollision()
{
	if (IsCollisionEnabled() && Composite.IsValid())
	{
		Composite->Detach();
		Flags.Clear(PHYS_ENT_COLLISION_ENABLED);
	}
}
//---------------------------------------------------------------------

} // namespace Physics
