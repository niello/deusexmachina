#include "PhysicsLevel.h"
#include <Physics/BulletConv.h>
#include <Physics/TickListener.h>
#include <Physics/PhysicsObject.h>
#include <Physics/PhysicsDebugDraw.h>
#include <Math/AABB.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/BroadphaseCollision/btAxisSweep3.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>

// TODO: probably newer bullet versions allow to clear or at least access all objects
class CMyDiscreteDynamicsWorld : public btDiscreteDynamicsWorld
{
public:

	CMyDiscreteDynamicsWorld(btDispatcher* dispatcher, btBroadphaseInterface* pairCache, btConstraintSolver* constraintSolver, btCollisionConfiguration* collisionConfiguration)
		: btDiscreteDynamicsWorld(dispatcher, pairCache, constraintSolver, collisionConfiguration)
	{}

	virtual ~CMyDiscreteDynamicsWorld() override
	{
		// TODO: or these objects are always stored in a game?
		//removeAction
		//removeCharacter
		//removeConstraint
		//removeVehicle
		//removeRigidBody

		// Necessary for static collision data removal, if loaded from .bullet
		// TODO: not tested, maybe a better way exists! For example store .bullet-loaded object refs in a game level.
		if (m_collisionObjects.size() > 0)
		{
			std::vector<btCollisionObject*> CollisionObjects(m_collisionObjects.size());
			for (int i = 0; i < m_collisionObjects.size(); ++i)
				CollisionObjects[i] = m_collisionObjects[i];

			for (btCollisionObject* pObject : CollisionObjects)
			{
				removeCollisionObject(pObject);
				delete pObject;
			}
		}
	}
};

class CClosestRayResultCallbackWithExclude: public btCollisionWorld::ClosestRayResultCallback
{
protected:

	Physics::CPhysicsObject* _pExclude = nullptr;

public:

	CClosestRayResultCallbackWithExclude(const btVector3& From, const btVector3& To, Physics::CPhysicsObject* pExclude):
		ClosestRayResultCallback(From, To), _pExclude(pExclude)
	{
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override
	{
		if (_pExclude && rayResult.m_collisionObject->getUserPointer() == _pExclude) return 1.f;
		return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
	}
};

namespace Physics
{

// btActionInterface could be of service, but it is executed just before PhysicsTick, after the simulation step
void CPhysicsLevel::BeforeTick(btDynamicsWorld* world, btScalar timeStep)
{
	auto pLevel = static_cast<CPhysicsLevel*>(world->getWorldUserInfo());
	for (auto pListener : pLevel->_TickListeners)
		pListener->BeforePhysicsTick(pLevel, timeStep);
}
//---------------------------------------------------------------------

void CPhysicsLevel::AfterTick(btDynamicsWorld* world, btScalar timeStep)
{
	auto pLevel = static_cast<CPhysicsLevel*>(world->getWorldUserInfo());
	for (auto pListener : pLevel->_TickListeners)
		pListener->AfterPhysicsTick(pLevel, timeStep);
}
//---------------------------------------------------------------------

CPhysicsLevel::CPhysicsLevel(const CAABB& Bounds)
{
	const btVector3 Min = VectorToBtVector(Bounds.Min);
	const btVector3 Max = VectorToBtVector(Bounds.Max);
	btBroadphaseInterface* pBtBroadPhase = new btAxisSweep3(Min, Max);

	btDefaultCollisionConfiguration* pBtCollCfg = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* pBtCollDisp = new btCollisionDispatcher(pBtCollCfg);

	//http://bulletphysics.org/mediawiki-1.5.8/index.php/BtContactSolverInfo
	btSequentialImpulseConstraintSolver* pBtSolver = new btSequentialImpulseConstraintSolver();

	pBtDynWorld = new CMyDiscreteDynamicsWorld(pBtCollDisp, pBtBroadPhase, pBtSolver, pBtCollCfg);

	pBtDynWorld->setGravity(btVector3(0.f, -9.81f, 0.f));

	pBtDynWorld->setInternalTickCallback(BeforeTick, this, true);
	pBtDynWorld->setInternalTickCallback(AfterTick, this, false);

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
}
//---------------------------------------------------------------------

CPhysicsLevel::~CPhysicsLevel()
{
	if (!pBtDynWorld) return;

	// FIXME: must delete all remaining objects in the world!

	btConstraintSolver* pBtSolver = pBtDynWorld->getConstraintSolver();
	btCollisionDispatcher* pBtCollDisp = (btCollisionDispatcher*)pBtDynWorld->getDispatcher();
	btCollisionConfiguration* pBtCollCfg = pBtCollDisp->getCollisionConfiguration();
	btBroadphaseInterface* pBtBroadPhase = pBtDynWorld->getBroadphase();

	delete pBtDynWorld;
	delete pBtSolver;
	delete pBtCollDisp;
	delete pBtCollCfg;
	delete pBtBroadPhase;
}
//---------------------------------------------------------------------

void CPhysicsLevel::Update(float dt)
{
	if (pBtDynWorld) pBtDynWorld->stepSimulation(dt, 10, StepTime);
}
//---------------------------------------------------------------------

void CPhysicsLevel::RenderDebug(Debug::CDebugDraw& DebugDraw)
{
	if (pBtDynWorld)
	{
		CPhysicsDebugDraw DD(DebugDraw);
		DD.setDebugMode(CPhysicsDebugDraw::DBG_DrawAabb | CPhysicsDebugDraw::DBG_DrawWireframe | CPhysicsDebugDraw::DBG_FastWireframe);
		pBtDynWorld->setDebugDrawer(&DD);
		pBtDynWorld->debugDrawWorld();
		pBtDynWorld->setDebugDrawer(nullptr);
	}
}
//---------------------------------------------------------------------

// pExclude is optional
bool CPhysicsLevel::GetClosestRayContact(const vector3& Start, const vector3& End, U16 Group, U16 Mask, vector3* pOutPos, PPhysicsObject* pOutObj, CPhysicsObject* pExclude) const
{
	if (!pBtDynWorld) FAIL;

	const btVector3 BtStart = VectorToBtVector(Start);
	const btVector3 BtEnd = VectorToBtVector(End);

	CClosestRayResultCallbackWithExclude RayCB(BtStart, BtEnd, pExclude);
	RayCB.m_collisionFilterGroup = Group;
	RayCB.m_collisionFilterMask = Mask;
	pBtDynWorld->rayTest(BtStart, BtEnd, RayCB);

	if (!RayCB.hasHit()) FAIL;

	if (pOutPos) *pOutPos = BtVectorToVector(RayCB.m_hitPointWorld);
	if (pOutObj) *pOutObj = RayCB.m_collisionObject ? static_cast<CPhysicsObject*>(RayCB.m_collisionObject->getUserPointer()) : nullptr;

	OK;
}
//---------------------------------------------------------------------

UPTR CPhysicsLevel::GetAllRayContacts(const vector3& Start, const vector3& End, U16 Group, U16 Mask) const
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

void CPhysicsLevel::RegisterTickListener(ITickListener* pListener)
{
	if (pListener) _TickListeners.insert(pListener);
}
//---------------------------------------------------------------------

void CPhysicsLevel::UnregisterTickListener(ITickListener* pListener)
{
	if (pListener) _TickListeners.erase(pListener);
}
//---------------------------------------------------------------------

void CPhysicsLevel::SetGravity(const vector3& NewGravity)
{
	n_assert(pBtDynWorld);
	pBtDynWorld->setGravity(VectorToBtVector(NewGravity));
}
//---------------------------------------------------------------------

void CPhysicsLevel::GetGravity(vector3& Out) const
{
	n_assert(pBtDynWorld);
	Out = BtVectorToVector(pBtDynWorld->getGravity());
}
//---------------------------------------------------------------------

}