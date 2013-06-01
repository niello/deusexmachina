#include "PhysicsWorldOld.h"

#include <Game/GameServer.h>
#include <Physics/Entity.h>
#include <Physics/Composite.h>
#include <Physics/Collision/Shape.h>
#include <Physics/PhysicsServerOld.h>
#include <Physics/RigidBodyOld.h>
#include <Audio/Event/PlaySound.h>
#include <Events/EventManager.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CPhysWorldOld, Core::CRefCounted);

CPhysWorldOld::CPhysWorldOld():
	TimeToSim(0.0),
	StepSize(0.01),
	CollisionSounds(0.0),
	ODEWorldID(0),
	ODEDynamicSpaceID(0),
	ODEStaticSpaceID(0),
	ODECommonSpaceID(0),
	ODERayID(0),
	ContactJointGroup(0),
	Gravity(0.0f, -9.81f, 0.0f)
{
}
//---------------------------------------------------------------------

CPhysWorldOld::~CPhysWorldOld()
{
	if (ODEWorldID) Term();
	n_assert(!ODEWorldID);
	n_assert(!ODEDynamicSpaceID);
	n_assert(!ODEStaticSpaceID);
	n_assert(!ODECommonSpaceID);
	n_assert(!ODERayID);
	n_assert(Shapes.GetCount() == 0);
	n_assert(Entities.GetCount() == 0);
}
//---------------------------------------------------------------------

// Called by Physics::Server when the Level is attached to the server.
bool CPhysWorldOld::Init(const bbox3& Bounds)
{
	TimeToSim = 0.0;

	// Initialize ODE //???per-Level?
	dInitODE();
	ODEWorldID = dWorldCreate();
	dWorldSetQuickStepNumIterations(ODEWorldID, 20);

	// FIXME(enno): is a quadtree significantly faster? -- can't count geoms with quadtree
	ODECommonSpaceID  = dSimpleSpaceCreate(NULL);
	ODEDynamicSpaceID = dSimpleSpaceCreate(ODECommonSpaceID);
	ODEStaticSpaceID  = dSimpleSpaceCreate(ODECommonSpaceID);

	dWorldSetGravity(ODEWorldID, Gravity.x, Gravity.y, Gravity.z);
	dWorldSetContactSurfaceLayer(ODEWorldID, 0.001f);
	dWorldSetContactMaxCorrectingVel(ODEWorldID, 100.0f);
	dWorldSetERP(ODEWorldID, 0.2f);     // ODE's default value
	dWorldSetCFM(ODEWorldID, 0.001f);    // the default is 10^-5

	// setup autodisabling
	dWorldSetAutoDisableFlag(ODEWorldID, 1);
	dWorldSetAutoDisableSteps(ODEWorldID, 5);
	//dWorldSetAutoDisableTime(ODEWorldID, 1.f);
	dWorldSetAutoDisableLinearThreshold(ODEWorldID, 0.05f);   // default is 0.01
	dWorldSetAutoDisableAngularThreshold(ODEWorldID, 0.1f);  // default is 0.01

	// create a Contact group for joints
	ContactJointGroup = dJointGroupCreate(0);

	OK;
}
//---------------------------------------------------------------------

// Called by Physics::Server when the Level is removed from the server.
void CPhysWorldOld::Term()
{
	n_assert(ODEWorldID);
	n_assert(ODEDynamicSpaceID);
	n_assert(ODEStaticSpaceID);
	n_assert(ODECommonSpaceID);

	for (int i = 0; i < Shapes.GetCount(); i++) Shapes[i]->Detach();
	Shapes.Clear();

	for (int i = 0; i < Entities.GetCount(); i++) Entities[i]->OnRemovedFromLevel();
	Entities.Clear();

	// delete the Contact group for joints
	dJointGroupDestroy(ContactJointGroup);

	// shutdown ode
	dSpaceDestroy(ODEDynamicSpaceID);
	dSpaceDestroy(ODEStaticSpaceID);
	dSpaceDestroy(ODECommonSpaceID);
	dWorldDestroy(ODEWorldID);
	dCloseODE();
	ODECommonSpaceID = NULL;
	ODEDynamicSpaceID = NULL;
	ODEStaticSpaceID = NULL;
	ODEWorldID = NULL;
}
//---------------------------------------------------------------------

//???INLINE?
void CPhysWorldOld::SetGravity(const vector3& NewGravity)
{
	Gravity = NewGravity;
	if (ODEWorldID) dWorldSetGravity(ODEWorldID, Gravity.x, Gravity.y, Gravity.z);
}
//---------------------------------------------------------------------

// Attach a static collide shape to the Level.
void CPhysWorldOld::AttachShape(Physics::CShape* pShape)
{
	n_assert(pShape);
	Shapes.Append(pShape);
	if (!pShape->Attach(ODEStaticSpaceID))
		n_error("CPhysWorldOld::AttachShape(): Failed attaching a shape!");
}
//---------------------------------------------------------------------

// Remove a static collide shape to the Level
void CPhysWorldOld::RemoveShape(CShape* pShape)
{
	n_assert(pShape);
	nArray<PShape>::iterator ShapeIt = Shapes.Find(pShape);
	n_assert(ShapeIt);
	pShape->Detach();
	Shapes.Erase(ShapeIt);
}
//---------------------------------------------------------------------

void CPhysWorldOld::AttachEntity(CEntity* pEnt)
{
	n_assert(pEnt);
	n_assert(pEnt->GetLevel() == 0);
	n_assert(!Entities.Find(pEnt));

	//!!!revisit!
	pEnt->OnAttachedToLevel(this);
	pEnt->Activate();
	
	Entities.Append(pEnt);
}
//---------------------------------------------------------------------

void CPhysWorldOld::RemoveEntity(CEntity* pEnt)
{
	n_assert(pEnt);
	n_assert(pEnt->GetLevel() == this);
	n_assert(pEnt->GetRefCount() > 0); //???can happen?
	nArray<PEntity>::iterator EntityIt = Entities.Find(pEnt);
	n_assert(EntityIt);
	Entities.Erase(EntityIt);
	pEnt->Deactivate();
	pEnt->OnRemovedFromLevel();
}
//---------------------------------------------------------------------

// Do a ray check starting from position `pos' along direction `dir'.
// Make resulting intersection points available in `GetIntersectionPoints()'.
bool CPhysWorldOld::RayTest(const vector3& Pos, const vector3& Dir)
{
	Contacts.Clear();

	float RayLength = Dir.len();
	if (RayLength < TINY) FAIL;

	ODERayID = dCreateRay(0, RayLength);
	vector3 NormDir = Dir / RayLength;
	dGeomRaySet(ODERayID, Pos.x, Pos.y, Pos.z, NormDir.x, NormDir.y, NormDir.z);
	dGeomRaySetLength(ODERayID, RayLength);
	dSpaceCollide2((dGeomID)ODECommonSpaceID, ODERayID, this, &ODERayCallback);
	dGeomDestroy(ODERayID);
	ODERayID = 0;

	return Contacts.GetCount() > 0;
}
//---------------------------------------------------------------------

const CContactPoint* CPhysWorldOld::GetClosestRayContact(const vector3& Pos, const vector3& Dir, DWORD SelfPhysID)
{
	RayTest(Pos, Dir);

	// Find closest contact
	int Idx = INVALID_INDEX;
	float ClosestDistanceSq = Dir.lensquared();
	for (int i = 0; i < Contacts.GetCount(); i++)
	{
		const CContactPoint& CurrContact = Contacts[i];
		if (SelfPhysID != -1 && CurrContact.EntityID == SelfPhysID) continue;
		float DistanceSq = (CurrContact.Position - Pos).lensquared();
		if (DistanceSq < ClosestDistanceSq)
		{
			Idx = i;
			ClosestDistanceSq = DistanceSq;
		}
	}
	return (Idx != INVALID_INDEX) ? &Contacts[Idx] : NULL;
}
//---------------------------------------------------------------------

// The "Near Callback". ODE calls this during collision detection to
// decide whether 2 geoms collide, and if yes, to generate Contact
// joints between the 2 involved rigid bodies.
void CPhysWorldOld::ODENearCallback(void* data, dGeomID o1, dGeomID o2)
{
	CPhysWorldOld* Level = (CPhysWorldOld*)data;

	// handle sub-spaces
	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		// collide a space with something
		dSpaceCollide2(o1, o2, data, &ODENearCallback);
		return;
	}

	// handle shape/shape collisions
	dBodyID Body1 = dGeomGetBody(o1);
	dBodyID Body2 = dGeomGetBody(o2);
	n_assert(Body1 != Body2);

	// do nothing if 2 bodies are connected by a joint
	if (Body1 && Body2 && dAreConnectedExcluding(Body1, Body2, dJointTypeContact))
	{
		// FIXME: bodies are connected, check if jointed-collision is enabled
		// for both bodies (whether 2 bodies connected by a joint should
		// collide or not, for this, both bodies must have set the
		// CollideConnected() flag set.
		CRigidBodyOld* PhysicsBody0 = (CRigidBodyOld*)dBodyGetData(Body1);
		n_assert(PhysicsBody0 && PhysicsBody0->IsInstanceOf(CRigidBodyOld::RTTI));
		if (!PhysicsBody0->CollideConnected) return;
		CRigidBodyOld* PhysicsBody1 = (CRigidBodyOld*) dBodyGetData(Body2);
		n_assert(PhysicsBody1 && PhysicsBody1->IsInstanceOf(CRigidBodyOld::RTTI));
		if (!PhysicsBody1->CollideConnected) return;
	}

	CShape* Shape1 = CShape::GetShapeFromGeom(o1);
	CShape* Shape2 = CShape::GetShapeFromGeom(o2);
	n_assert(Shape1 && Shape2);
	n_assert(!((Shape1->GetType() == CShape::Mesh) && (Shape2->GetType() == CShape::Mesh)));

	// initialize Contact array
	bool MaterialsValid = (Shape1->GetMaterialType() != InvalidMaterial &&  Shape2->GetMaterialType() != InvalidMaterial);
	float Friction;
	float Bounce;
	if (MaterialsValid)
	{
		Friction = Physics::CMaterialTable::GetFriction(Shape1->GetMaterialType(), Shape2->GetMaterialType());
		Bounce = Physics::CMaterialTable::GetBounce(Shape1->GetMaterialType(), Shape2->GetMaterialType());
	}
	else
	{
		Friction = 0.f;
		Bounce = 0.f;
	}

	static dContact Contact[MaxContacts];
	for (int i = 0; i < MaxContacts; i++)
	{
		Contact[i].surface.mode = dContactBounce | dContactSoftCFM;
		Contact[i].surface.mu = Friction;
		Contact[i].surface.mu2 = 0.0f;
		Contact[i].surface.bounce = Bounce;
		Contact[i].surface.bounce_vel = 1.0f;
		Contact[i].surface.soft_cfm = 0.0001f;
		Contact[i].surface.soft_erp = 0.2f;
	}

	// do collision detection
	int CollisionCount = dCollide(o1, o2, MaxContacts, &(Contact[0].geom), sizeof(dContact));

	//???!!!set in OnCollision?!
	Shape1->SetNumCollisions(Shape1->GetNumCollisions() + CollisionCount);
	Shape2->SetNumCollisions(Shape2->GetNumCollisions() + CollisionCount);
	
	if (CollisionCount > 0)
	{
		if (!Shape1->OnCollide(Shape2) || !Shape2->OnCollide(Shape1)) return;

			// create a Contact for each collision
		for (int i = 0; i < CollisionCount; i++)
			dJointAttach(
				dJointCreateContact(Level->ODEWorldID, Level->ContactJointGroup, Contact + i), Body1, Body2);
	}

	//???sounds here or in sound system on event?
	// FIXME: not really ready for prime Time
	// TODO: implement roll / slide sounds (sounds that stop as soon as the Contact is gone)
	//       roll / slide sounds also need to consider relative velocity
	nTime Now = GameSrv->GetTime();
	if (CollisionCount != 0)
	{
		CShape* Key[2];

		// build an unique Key for every colliding shape combination
		if (Shape1 < Shape2)
		{
			Key[0] = Shape1;
			Key[1] = Shape2;
		}
		else
		{
			Key[0] = Shape2;
			Key[1] = Shape1;
		}

		if ((Now - Level->CollisionSounds.At(Key, sizeof(Key))) > 0.25f)
		{
			CRigidBodyOld* Rigid1 = Shape1->GetRigidBody();
			CRigidBodyOld* Rigid2 = Shape2->GetRigidBody();

			if ((!Rigid1 || !Rigid1->IsEnabled()) && (!Rigid2 || !Rigid2->IsEnabled())) return;

			nString Sound;
			if (MaterialsValid)
				Physics::CMaterialTable::GetCollisionSound(Shape1->GetMaterialType(), Shape2->GetMaterialType());

			if (Sound.IsValid())
			{
				vector3 Normal;
				CPhysicsServerOld::OdeToVector3(Contact[0].geom.normal, Normal);
				vector3 Velocity = Rigid1 ? Rigid1->GetLinearVelocity() : vector3(0.0f, 0.0f, 0.0f);
				if (Rigid2) Velocity -= Rigid2->GetLinearVelocity();

				float Volume = n_saturate((-Velocity.dot(Normal) - 0.3f) / 4.0f);
				if (Volume > 0.0f)
				{
					Ptr<Event::PlaySound> Evt = Event::PlaySound::CreateInstance();
					Evt->Name = Sound;
					Evt->Position.set(Contact[0].geom.pos[0], Contact[0].geom.pos[1], Contact[0].geom.pos[2]);
					Evt->Volume = Volume;
					EventMgr->FireEvent(*Evt);
					Level->CollisionSounds.At(Key, sizeof(Key)) = Now;
				}
			}
		}
	}
}
//---------------------------------------------------------------------

void CPhysWorldOld::ODERayCallback(void* data, dGeomID o1, dGeomID o2)
{
	n_assert(data);
	n_assert(o1 != o2);

	// handle sub-space
	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2(o1, o2, data, &ODERayCallback);
		return;
	}

	CPhysWorldOld* pLevel = (CPhysWorldOld*)data;
	dGeomID ODERayID = pLevel->ODERayID;

	// check for exclusion
	CShape* pOtherShape;
	if (o1 == ODERayID) pOtherShape = CShape::GetShapeFromGeom(o2);
	else if (o2 == ODERayID) pOtherShape = CShape::GetShapeFromGeom(o1);
	else n_error("Unexpected ray collision without a ray!");

	dContactGeom ODEContact[MaxContacts];
	int CollCount = (o1 == ODERayID) ?
		dCollide(o2, o1, MaxContacts, ODEContact, sizeof(dContactGeom)) :
		dCollide(o1, o2, MaxContacts, ODEContact, sizeof(dContactGeom));

	CContactPoint ContactPt;
	for (int i = 0; i < CollCount; i++)
	{
		dVector3 ODEOrigin, ODEDir;
		dGeomRayGet(ODERayID, ODEOrigin, ODEDir);

		vector3 Origin, Dir;
		CPhysicsServerOld::OdeToVector3(ODEOrigin, Origin);
		CPhysicsServerOld::OdeToVector3(ODEDir, Dir);

		// FIXME: hmm, ODEContact[x].geom.pos[] doesn't seem to be correct with mesh
		// shapes which are not at the origin. Computing the intersection pos from
		// the stabbing depth and the ray's original vector
		ContactPt.Position = Origin + Dir * ODEContact[i].depth;
		ContactPt.UpVector.set(ODEContact[i].normal[0], ODEContact[i].normal[1], ODEContact[i].normal[2]);
		ContactPt.Depth = ODEContact[i].depth;
		CEntity* pOtherEntity = pOtherShape->GetEntity();
		ContactPt.EntityID = pOtherEntity ? pOtherEntity->GetUID() : 0;
		CRigidBodyOld* pOtherRB = pOtherShape->GetRigidBody();
		ContactPt.RigidBodyID = pOtherRB ? pOtherRB->GetUID() : 0;
		ContactPt.Material = pOtherShape->GetMaterialType();
		pLevel->Contacts.Append(ContactPt);
	}
}
//---------------------------------------------------------------------

// Physics simulation is triggered using a constant step size
void CPhysWorldOld::Trigger(float FrameTime)
{
	for (int i = 0; i < Entities.GetCount(); i++) Entities[i]->OnFrameBefore();

	TimeToSim += GameSrv->GetFrameTime();
	while (TimeToSim > StepSize)
	{
		for (int i = 0; i < Entities.GetCount(); i++) Entities[i]->OnStepBefore();

		dSpaceCollide2((dGeomID)ODEDynamicSpaceID, (dGeomID)ODEStaticSpaceID, this, &ODENearCallback);
		dSpaceCollide(ODEDynamicSpaceID, this, &ODENearCallback);
		dWorldQuickStep(ODEWorldID, dReal(StepSize));
		dJointGroupEmpty(ContactJointGroup);

		for (int i = 0; i < Entities.GetCount(); i++) Entities[i]->OnStepAfter();

		TimeToSim -= StepSize;
    }

	for (int i = 0; i < Entities.GetCount(); i++) Entities[i]->OnFrameAfter();
}
//---------------------------------------------------------------------

void CPhysWorldOld::RenderDebug()
{
	for (int i = 0; i < Shapes.GetCount(); i++) Shapes[i]->RenderDebug(matrix44::identity);
	for (int i = 0; i < Entities.GetCount(); i++) Entities[i]->RenderDebug();
}
//---------------------------------------------------------------------

} // namespace Physics
