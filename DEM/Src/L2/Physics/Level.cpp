#include "Level.h"

#include <Game/GameServer.h>
#include <Physics/Entity.h>
#include <Physics/Composite.h>
#include <Physics/Collision/Shape.h>
#include <Physics/PhysicsServer.h>
#include <Physics/RigidBody.h>
#include <Audio/Event/PlaySound.h>
#include <Events/EventManager.h>

using namespace Core;

namespace Physics
{
ImplementRTTI(Physics::CLevel, Core::CRefCounted);
ImplementFactory(Physics::CLevel);

CLevel::CLevel() :
#ifdef DEM_STATS
    statsNumSpaceCollideCalled(0),
    statsNumNearCallbackCalled(0),
    statsNumCollideCalled(0),
    statsNumCollided(0),
    statsNumSpaces(0),
    statsNumShapes(0),
    statsNumSteps(0),
#endif
	TimeToSim(0.0),
	StepSize(0.01),
	CollisionSounds(0.0),
	ODEWorldID(0),
	ODEDynamicSpaceID(0),
	ODEStaticSpaceID(0),
	ODECommonSpaceID(0),
	ContactJointGroup(0),
	Gravity(0.0f, -9.81f, 0.0f)
{
	PROFILER_INIT(profFrameBefore, "profMangaPhysFrameBefore");
	PROFILER_INIT(profFrameAfter, "profMangaPhysFrameAfter");
	PROFILER_INIT(profStepBefore, "profMangaPhysStepBefore");
	PROFILER_INIT(profStepAfter, "profMangaPhysStepAfter");
	PROFILER_INIT(profCollide, "profMangaPhysCollide");
	PROFILER_INIT(profStep, "profMangaPhysStep");
	PROFILER_INIT(profJointGroupEmpty, "profMangaPhysJointGroupEmpty");
}
//---------------------------------------------------------------------

CLevel::~CLevel()
{
	n_assert(!ODEWorldID);
	n_assert(!ODEDynamicSpaceID);
	n_assert(!ODEStaticSpaceID);
	n_assert(!ODECommonSpaceID);
	n_assert(Shapes.Size() == 0);
	n_assert(Entities.Size() == 0);
}
//---------------------------------------------------------------------

// Called by Physics::Server when the Level is attached to the server.
void CLevel::Activate()
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
}
//---------------------------------------------------------------------

// Called by Physics::Server when the Level is removed from the server.
void CLevel::Deactivate()
{
	n_assert(ODEWorldID);
	n_assert(ODEDynamicSpaceID);
	n_assert(ODEStaticSpaceID);
	n_assert(ODECommonSpaceID);

	for (int i = 0; i < Shapes.Size(); i++) Shapes[i]->Detach();
	Shapes.Clear();

	for (int i = 0; i < Entities.Size(); i++) Entities[i]->OnRemovedFromLevel();
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
void CLevel::SetGravity(const vector3& NewGravity)
{
	Gravity = NewGravity;
	if (ODEWorldID) dWorldSetGravity(ODEWorldID, Gravity.x, Gravity.y, Gravity.z);
}
//---------------------------------------------------------------------

// Attach a static collide shape to the Level.
void CLevel::AttachShape(Physics::CShape* pShape)
{
	n_assert(pShape);
	Shapes.Append(pShape);
	if (!pShape->Attach(ODEStaticSpaceID))
		n_error("CLevel::AttachShape(): Failed attaching a shape!");
}
//---------------------------------------------------------------------

// Remove a static collide shape to the Level
void CLevel::RemoveShape(CShape* pShape)
{
	n_assert(pShape);
	nArray<PShape>::iterator ShapeIt = Shapes.Find(pShape);
	n_assert(ShapeIt);
	pShape->Detach();
	Shapes.Erase(ShapeIt);
}
//---------------------------------------------------------------------

void CLevel::AttachEntity(CEntity* pEnt)
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

void CLevel::RemoveEntity(CEntity* pEnt)
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

// The "Near Callback". ODE calls this during collision detection to
// decide whether 2 geoms collide, and if yes, to generate Contact
// joints between the 2 involved rigid bodies.
void CLevel::ODENearCallback(void* data, dGeomID o1, dGeomID o2)
{
	CLevel* Level = (CLevel*)data;
	Level->statsNumNearCallbackCalled++;

	// handle sub-spaces
	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		// collide a space with something
		Level->statsNumSpaceCollideCalled++;
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
		CRigidBody* PhysicsBody0 = (CRigidBody*)dBodyGetData(Body1);
		n_assert(PhysicsBody0 && PhysicsBody0->IsInstanceOf(CRigidBody::RTTI));
		if (!PhysicsBody0->CollideConnected) return;
		CRigidBody* PhysicsBody1 = (CRigidBody*) dBodyGetData(Body2);
		n_assert(PhysicsBody1 && PhysicsBody1->IsInstanceOf(CRigidBody::RTTI));
		if (!PhysicsBody1->CollideConnected) return;
	}

	CShape* Shape1 = CShape::GetShapeFromGeom(o1);
	CShape* Shape2 = CShape::GetShapeFromGeom(o2);
	n_assert(Shape1 && Shape2);
	n_assert(!((Shape1->GetType() == CShape::Mesh) && (Shape2->GetType() == CShape::Mesh)));

	Level->statsNumCollideCalled++;

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
		Level->statsNumCollided++;

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
			CRigidBody* Rigid1 = Shape1->GetRigidBody();
			CRigidBody* Rigid2 = Shape2->GetRigidBody();

			if ((!Rigid1 || !Rigid1->IsEnabled()) && (!Rigid2 || !Rigid2->IsEnabled())) return;

			nString Sound;
			if (MaterialsValid)
				Physics::CMaterialTable::GetCollisionSound(Shape1->GetMaterialType(), Shape2->GetMaterialType());

			if (Sound.IsValid())
			{
				vector3 Normal;
				CPhysicsServer::OdeToVector3(Contact[0].geom.normal, Normal);
				vector3 Velocity = Rigid1 ? Rigid1->GetLinearVelocity() : vector3(0.0f, 0.0f, 0.0f);
				if (Rigid2) Velocity -= Rigid2->GetLinearVelocity();

				float Volume = n_saturate((-Velocity.dot(Normal) - 0.3f) / 4.0f);
				if (Volume > 0.0f)
				{
					Ptr<Event::PlaySound> Evt = Event::PlaySound::Create();
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

//Trigger the ODE simulation. This method should be called frequently
//(call SetTime() before invoking Trigger() to update the current Time).
//The method will invoke dWorldStep one or several times, depending
//on the Time since the last call, and the step size of the Level.
//The method will make sure that the physics simulation is triggered
//using a constant step size.
void CLevel::Trigger()
{
	PROFILER_START(profFrameBefore);
	for (int i = 0; i < Entities.Size(); i++) Entities[i]->OnFrameBefore();
	PROFILER_STOP(profFrameBefore);

	PROFILER_RESET(profStepBefore);
	PROFILER_RESET(profStepAfter);
	PROFILER_RESET(profCollide);
	PROFILER_RESET(profStep);
	PROFILER_RESET(profJointGroupEmpty);

#ifdef DEM_STATS
	statsNumNearCallbackCalled = 0;
	statsNumCollideCalled = 0;
	statsNumCollided = 0;
	statsNumSpaceCollideCalled = 0;
	statsNumSteps = 0;
#endif

	TimeToSim += GameSrv->GetFrameTime();
	while (TimeToSim > StepSize)
	{
		PROFILER_STARTACCUM(profStepBefore);
		for (int i = 0; i < Entities.Size(); i++) Entities[i]->OnStepBefore();
		PROFILER_STOPACCUM(profStepBefore);

		// do collision detection
		PROFILER_STARTACCUM(profCollide);
		statsNumSpaceCollideCalled++;
		dSpaceCollide2((dGeomID)ODEDynamicSpaceID, (dGeomID)ODEStaticSpaceID, this, &ODENearCallback);
		dSpaceCollide(ODEDynamicSpaceID, this, &ODENearCallback);
		PROFILER_STOPACCUM(profCollide);

		// step physics simulation
		PROFILER_STARTACCUM(profStep);
		dWorldQuickStep(ODEWorldID, dReal(StepSize));
		PROFILER_STOPACCUM(profStep);

		// clear Contact joints
		PROFILER_STARTACCUM(profJointGroupEmpty);
		dJointGroupEmpty(ContactJointGroup);
		PROFILER_STOPACCUM(profJointGroupEmpty);

		PROFILER_STARTACCUM(profStepAfter);
		for (int i = 0; i < Entities.Size(); i++) Entities[i]->OnStepAfter();
		PROFILER_STOPACCUM(profStepAfter);

		statsNumSteps++;
		TimeToSim -= StepSize;
    }

	// export statistics
#ifdef DEM_STATS
	//nWatched watchSpaceCollideCalled("statsMangaPhysicsSpaceCollideCalled", DATA_TYPE(int));
	//nWatched watchNearCallbackCalled("statsMangaPhysicsNearCallbackCalled", DATA_TYPE(int));
	//nWatched watchCollideCalled("statsMangaPhysicsCollideCalled", DATA_TYPE(int));
	//nWatched watchCollided("statsMangaPhysicsCollided", DATA_TYPE(int));
	//nWatched watchSpaces("statsMangaPhysicsSpaces", DATA_TYPE(int));
	//nWatched watchShapes("statsMangaPhysicsShapes", DATA_TYPE(int));
	//nWatched watchSteps("statsMangaPhysicsSteps", DATA_TYPE(int));
	//if (statsNumSteps > 0)
	//{
	//	watchSpaceCollideCalled->SetValue(statsNumSpaceCollideCalled/statsNumSteps);
	//	watchNearCallbackCalled->SetValue(statsNumNearCallbackCalled/statsNumSteps);
	//	watchCollideCalled->SetValue(statsNumCollideCalled/statsNumSteps);
	//	watchCollided->SetValue(statsNumCollided/statsNumSteps);
	//}
	//watchSpaces->SetValue(statsNumSpaces);
	//watchShapes->SetValue(statsNumShapes);
	//watchSteps->SetValue(statsNumSteps);
#endif

	// invoke the "on-frame-after" methods
	PROFILER_START(profFrameAfter);
	for (int i = 0; i < Entities.Size(); i++) Entities[i]->OnFrameAfter();
	PROFILER_STOP(profFrameAfter);
}
//---------------------------------------------------------------------

void CLevel::RenderDebug()
{
	for (int i = 0; i < Shapes.Size(); i++) Shapes[i]->RenderDebug(matrix44::identity);
	for (int i = 0; i < Entities.Size(); i++) Entities[i]->RenderDebug();
}
//---------------------------------------------------------------------

} // namespace Physics
