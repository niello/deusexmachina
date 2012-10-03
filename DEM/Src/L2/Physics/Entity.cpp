#include "Entity.h"

#include <Physics/Composite.h>
#include <Physics/Level.h>

namespace Physics
{
ImplementRTTI(Physics::CEntity, Core::CRefCounted);
ImplementFactory(Physics::CEntity);

DWORD CEntity::UIDCounter = 1;

CEntity::CEntity(): Level(NULL), UserData(CStrID::Empty), Stamp(0)
{
	UID = UIDCounter++;
	CollidedShapes.SetFlags(nArray<PShape>::DoubleGrowSize);
	PhysicsSrv->RegisterEntity(this);
}
//---------------------------------------------------------------------

CEntity::~CEntity()
{
	n_assert(!Level);
	if (IsActive()) Deactivate();
	PhysicsSrv->UnregisterEntity(this);
}
//---------------------------------------------------------------------

void CEntity::Activate()
{
	n_assert(!IsActive());
	if (!Composite.isvalid())
	{
		if (CompositeName.IsValid())
			SetComposite(PhysicsSrv->LoadCompositeFromPRM(CompositeName));
		else n_error("Physics::CEntity: no valid physics Composite name given!");
	}
	else Composite->SetTransform(Transform);

	EnableCollision();

	Flags.Set(PHYS_ENT_ACTIVE);
}
//---------------------------------------------------------------------

void CEntity::Deactivate()
{
	n_assert(IsActive());
	DisableCollision();
	Flags.Clear(PHYS_ENT_ACTIVE);
}
//---------------------------------------------------------------------

void CEntity::OnAttachedToLevel(CLevel* pLevel)
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
	if (Composite.isvalid()) Composite->SetTransform(Tfm);
	Transform = Tfm;
}
//---------------------------------------------------------------------

//???INLINE?
// The transformation is updated during Physics::Server::Trigger().
matrix44 CEntity::GetTransform() const
{
	return (Composite.isvalid()) ? Composite->GetTransform() : Transform;
}
//---------------------------------------------------------------------

//???INLINE?
// Return true if the transformation has changed during the frame.
bool CEntity::HasTransformChanged() const
{
	return Composite.isvalid() && Composite->HasTransformChanged();
}
//---------------------------------------------------------------------

//???INLINE?
vector3 CEntity::GetVelocity() const
{
	return (Composite.isvalid() && Composite->GetMasterBody()) ?
		Composite->GetMasterBody()->GetLinearVelocity() :
		vector3::Zero;
}
//---------------------------------------------------------------------

//???INLINE?
void CEntity::OnStepBefore()
{
	if (Composite.isvalid()) Composite->OnStepBefore();
}
//---------------------------------------------------------------------

void CEntity::OnStepAfter()
{
	if (Composite.isvalid()) Composite->OnStepAfter();
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
	if (Composite.isvalid()) Composite->OnFrameBefore();
}
//---------------------------------------------------------------------

void CEntity::OnFrameAfter()
{
	if (Composite.isvalid()) Composite->OnFrameAfter();
}
//---------------------------------------------------------------------

//???INLINE?
void CEntity::Reset()
{
	if (Composite.isvalid()) Composite->Reset();
}
//---------------------------------------------------------------------

//???INLINE?
int CEntity::GetNumCollisions() const
{
	return (Composite.isvalid()) ? Composite->GetNumCollisions() : 0;
}
//---------------------------------------------------------------------

//???INLINE?
bool CEntity::IsHorizontalCollided() const
{
	return Composite.isvalid() && Composite->IsHorizontalCollided();
}
//---------------------------------------------------------------------

//???INLINE?
// A disabled entity will enable itself automatically on contact with other enabled entities.
void CEntity::SetEnabled(bool Enabled)
{
	if (Composite.isvalid()) Composite->SetEnabled(Enabled);
}
//---------------------------------------------------------------------

bool CEntity::IsEnabled() const
{
	return Composite.isvalid() && Composite->IsEnabled();
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
	if (Composite.isvalid()) Composite->RenderDebug();
}
//---------------------------------------------------------------------

//???rename?
void CEntity::EnableCollision()
{
	if (!IsCollisionEnabled() && Composite.isvalid())
	{
		Composite->Attach(Level->GetODEWorldID(), Level->GetODEDynamicSpaceID(), Level->GetODEStaticSpaceID());
		Flags.Set(PHYS_ENT_COLLISION_ENABLED);
	}
}
//---------------------------------------------------------------------

//???rename?
void CEntity::DisableCollision()
{
	if (IsCollisionEnabled() && Composite.isvalid())
	{
		Composite->Detach();
		Flags.Clear(PHYS_ENT_COLLISION_ENABLED);
	}
}
//---------------------------------------------------------------------

} // namespace Physics
