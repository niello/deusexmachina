#include "PhysicsWorld.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsDebugDraw.h>
#include <Math/AABB.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/BroadphaseCollision/btAxisSweep3.h>
//#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CPhysicsWorld, Core::CObject);

CPhysicsWorld::~CPhysicsWorld()
{
	Term();
}
//---------------------------------------------------------------------

// Called by Physics::Server when the Level is attached to the server.
bool CPhysicsWorld::Init(const CAABB& Bounds)
{
	n_assert(!pBtDynWorld);

	btVector3 Min = VectorToBtVector(Bounds.Min);
	btVector3 Max = VectorToBtVector(Bounds.Max);
	btBroadphaseInterface* pBtBroadPhase = new btAxisSweep3(Min, Max);
	//btBroadphaseInterface* pBtBroadPhase = new btDbvtBroadphase();

	btDefaultCollisionConfiguration* pBtCollCfg = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* pBtCollDisp = new btCollisionDispatcher(pBtCollCfg);

	//http://bulletphysics.org/mediawiki-1.5.8/index.php/BtContactSolverInfo
	btSequentialImpulseConstraintSolver* pBtSolver = new btSequentialImpulseConstraintSolver;

	pBtDynWorld = new btDiscreteDynamicsWorld(pBtCollDisp, pBtBroadPhase, pBtSolver, pBtCollCfg);

	pBtDynWorld->setGravity(btVector3(0.f, -9.81f, 0.f));
	pBtDynWorld->setDebugDrawer(PhysicsSrv->GetDebugDrawer());

	//!!!can reimplement with some notifications like FireEvent(OnCollision)!
	//btGhostPairCallback* pGhostPairCB = new btGhostPairCallback(); //!!!delete!
	//pBtDynWorld->getPairCache()->setInternalGhostPairCallback(pGhostPairCB);

	//struct ContactSensorCallback : public btCollisionWorld::ContactResultCallback 
	//world->contactTest(pCollObj, ContactSensorCallback());
	//Collision objects with a callback still have collision response with dynamic rigid bodies. In order to use collision objects as trigger, you have to disable the collision response. 
	//mBody->setCollisionFlags(mBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE));

	/*
	btGhostObject for triggers and raytests, it keeps track of all its intersections
	btGhostPairCallback to notify application of new/removed collisions
	???btBvhTriangleMeshShape->performRaycast(btTriangleCallback*)
	bt[Scaled]BvhTriangleMeshShape for all the non-modifyable and non-deletable static environment except a terrain
	it can be created at compile time and saved (btOptimizedBvh, Demo/ConcaveDemo)
	a->group in b->mask && b->group in a->mask = collision, 32bit
	btOverlapFilterCallback - broadphase, receives broadphase proxies (AABBs?) world->getpaircache->set
	near callback dispatcher->set
	appendAnchor attaches body to body
	use CCD for fast moving objects
	btConeTwistConstraint for ragdolls
	#define BT_NO_PROFILE 1 in release
	materials - restitution and friction
	there is a multithreading support
	*/

	OK;
}
//---------------------------------------------------------------------

void CPhysicsWorld::Term()
{
	if (pBtDynWorld)
	{
		for (int i = 0; i < Objects.GetCount(); ++i)
			Objects[i]->RemoveFromLevel();

		btConstraintSolver* pBtSolver = pBtDynWorld->getConstraintSolver();
		btCollisionDispatcher* pBtCollDisp = (btCollisionDispatcher*)pBtDynWorld->getDispatcher();
		btCollisionConfiguration* pBtCollCfg = pBtCollDisp->getCollisionConfiguration();
		btBroadphaseInterface* pBtBroadPhase = pBtDynWorld->getBroadphase();

		delete pBtDynWorld;
		delete pBtSolver;
		delete pBtCollDisp;
		delete pBtCollCfg;
		delete pBtBroadPhase;

		pBtDynWorld = NULL;
	}

	Objects.Clear();
}
//---------------------------------------------------------------------

void CPhysicsWorld::Trigger(float FrameTime)
{
	n_assert(pBtDynWorld);
	pBtDynWorld->stepSimulation(FrameTime, 10, StepTime);
}
//---------------------------------------------------------------------

void CPhysicsWorld::RenderDebug()
{
	n_assert(pBtDynWorld);
	pBtDynWorld->debugDrawWorld();
}
//---------------------------------------------------------------------

bool CPhysicsWorld::AddCollisionObject(CPhysicsObject& Obj)
{
	n_assert(pBtDynWorld && Obj.GetBtObject());
	Objects.Add(&Obj);
	OK;
}
//---------------------------------------------------------------------

void CPhysicsWorld::RemoveCollisionObject(CPhysicsObject& Obj)
{
	Objects.RemoveByValue(&Obj);
}
//---------------------------------------------------------------------

bool CPhysicsWorld::GetClosestRayContact(const vector3& Start, const vector3& End, U16 Group, U16 Mask, vector3* pOutPos, PPhysicsObj* pOutObj) const
{
	n_assert(pBtDynWorld);

	btVector3 BtStart = VectorToBtVector(Start);
	btVector3 BtEnd = VectorToBtVector(End);
	btCollisionWorld::ClosestRayResultCallback RayCB(BtStart, BtEnd);
	RayCB.m_collisionFilterGroup = Group;
	RayCB.m_collisionFilterMask = Mask;
	pBtDynWorld->rayTest(BtStart, BtEnd, RayCB);

	if (!RayCB.hasHit()) FAIL;

	if (pOutPos) *pOutPos = BtVectorToVector(RayCB.m_hitPointWorld);
	if (pOutObj) *pOutObj = RayCB.m_collisionObject ? (CPhysicsObject*)RayCB.m_collisionObject->getUserPointer() : NULL;
	n_assert_dbg(!pOutObj || !(*pOutObj).IsValidPtr() || Objects.Contains(*pOutObj));

	OK;
}
//---------------------------------------------------------------------

DWORD CPhysicsWorld::GetAllRayContacts(const vector3& Start, const vector3& End, U16 Group, U16 Mask) const
{
	n_assert(pBtDynWorld);

	btVector3 BtStart = VectorToBtVector(Start);
	btVector3 BtEnd = VectorToBtVector(End);
	btCollisionWorld::AllHitsRayResultCallback RayCB(BtStart, BtEnd);
	RayCB.m_collisionFilterGroup = Group;
	RayCB.m_collisionFilterMask = Mask;
	pBtDynWorld->rayTest(BtStart, BtEnd, RayCB);

	if (!RayCB.hasHit()) return 0;

	// Create contact points
	//btVector3 Point = RayCB.m_hitPointWorld;
	//btVector3 Normal = RayCB.m_hitNormalWorld;
	//const btCollisionObject* pCollObj = RayCB.m_collisionObjects;
	//void* pUserObj = pCollObj->getUserPointer();

	return RayCB.m_collisionObjects.size();
}
//---------------------------------------------------------------------

void CPhysicsWorld::SetGravity(const vector3& NewGravity)
{
	n_assert(pBtDynWorld);
	pBtDynWorld->setGravity(VectorToBtVector(NewGravity));
}
//---------------------------------------------------------------------

void CPhysicsWorld::GetGravity(vector3& Out) const
{
	n_assert(pBtDynWorld);
	Out = BtVectorToVector(pBtDynWorld->getGravity());
}
//---------------------------------------------------------------------

}