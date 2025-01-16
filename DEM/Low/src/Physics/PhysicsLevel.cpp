#include "PhysicsLevel.h"
#include <Physics/BulletConv.h>
#include <Physics/TickListener.h>
#include <Physics/PhysicsObject.h>
#include <Physics/PhysicsDebugDraw.h>
#include <Math/AABB.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
//#include <BulletCollision/BroadphaseCollision/btAxisSweep3.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
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

class CFunctorContactCallback : public btCollisionWorld::ContactResultCallback
{
protected:

	btCollisionObject& _Self;
	UPTR               _Counter = 0;
	std::function<bool(Physics::CPhysicsObject&, const rtm::vector4f&)>& _Callback;

public:

	//???struct CContact instead of separate object + pos? May add normal.
	CFunctorContactCallback(btCollisionObject& Self, std::function<bool(Physics::CPhysicsObject&, const rtm::vector4f&)>& Callback)
		: _Self(Self), _Callback(Callback)
	{
		if (const btBroadphaseProxy* pBroadphase = _Self.getBroadphaseHandle())
		{
			m_collisionFilterGroup = pBroadphase->m_collisionFilterGroup;
			m_collisionFilterMask = pBroadphase->m_collisionFilterMask;
		}
	}

	CFunctorContactCallback(btCollisionObject& Self, U32 Group, U32 Mask, std::function<bool(Physics::CPhysicsObject&, const rtm::vector4f&)>& Callback)
		: _Self(Self), _Callback(Callback)
	{
		m_collisionFilterGroup = Group;
		m_collisionFilterMask = Mask;
	}

	virtual btScalar addSingleResult(btManifoldPoint& cp,
		const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
		const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		// NB: the same object can be reported multiple times
		const bool IsMeFirst = (colObj0Wrap->getCollisionObject() == &_Self);
		const auto pBtObj = IsMeFirst ? colObj1Wrap->getCollisionObject() : colObj0Wrap->getCollisionObject();
		const auto& Pos = IsMeFirst ? cp.m_positionWorldOnB : cp.m_positionWorldOnA;
		if (auto pPhysicsObj = static_cast<Physics::CPhysicsObject*>(pBtObj->getUserPointer()))
		{
			++_Counter;
			_Callback(*pPhysicsObj, Math::FromBullet(Pos)); // FIXME: how to interrupt query execution?
		}
		return 0;
	}

	UPTR GetContactCount() const { return _Counter; }
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

CPhysicsLevel::CPhysicsLevel(const Math::CAABB& /*Bounds*/)
{
	// Register predefined collision groups
	PredefinedCollisionGroups.Dynamic = CollisionGroups.GetMask("Dynamic");
	PredefinedCollisionGroups.Static = CollisionGroups.GetMask("Static");
	PredefinedCollisionGroups.Interactable = CollisionGroups.GetMask("Interactable");
	PredefinedCollisionGroups.Query = CollisionGroups.GetMask("Query");
	PredefinedCollisionGroups.All = CollisionGroups.SetAlias(CStrID("All"), std::numeric_limits<U32>().max());

	// TODO PERF: test, DBVT is faster according to https://web.archive.org/web/20170704124316/http://bulletphysics.org/mediawiki-1.5.8/index.php/CDTestFramework
	//const btVector3 Min = Math::ToBullet3(rtm::vector_sub(Bounds.Center, Bounds.Extent));
	//const btVector3 Max = Math::ToBullet3(rtm::vector_add(Bounds.Center, Bounds.Extent));
	//btBroadphaseInterface* pBtBroadPhase = new btAxisSweep3(Min, Max);
	btBroadphaseInterface* pBtBroadPhase = new btDbvtBroadphase();

	btDefaultCollisionConfiguration* pBtCollCfg = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* pBtCollDisp = new btCollisionDispatcher(pBtCollCfg);

	//http://bulletphysics.org/mediawiki-1.5.8/index.php/BtContactSolverInfo
	btSequentialImpulseConstraintSolver* pBtSolver = new btSequentialImpulseConstraintSolver();

	pBtDynWorld = new CMyDiscreteDynamicsWorld(pBtCollDisp, pBtBroadPhase, pBtSolver, pBtCollCfg);

	pBtDynWorld->setGravity(btVector3(0.f, -9.81f, 0.f));

	pBtDynWorld->setInternalTickCallback(BeforeTick, this, true);
	pBtDynWorld->setInternalTickCallback(AfterTick, this, false);

	/*
	???btBvhTriangleMeshShape->performRaycast(btTriangleCallback*)
	bt[Scaled]BvhTriangleMeshShape for all the non-modifyable and non-deletable static environment except a terrain
	it can be created at compile time and saved (btOptimizedBvh, Demo/ConcaveDemo)
	appendAnchor attaches body to body
	use CCD for fast moving objects
	btConeTwistConstraint for ragdolls
	materials - restitution and friction
	there is a multithreading support
	*/
}
//---------------------------------------------------------------------

CPhysicsLevel::~CPhysicsLevel()
{
	if (!pBtDynWorld) return;

	btConstraintSolver* pBtSolver = pBtDynWorld->getConstraintSolver();
	btCollisionDispatcher* pBtCollDisp = (btCollisionDispatcher*)pBtDynWorld->getDispatcher();
	btCollisionConfiguration* pBtCollCfg = pBtCollDisp->getCollisionConfiguration();
	btBroadphaseInterface* pBtBroadPhase = pBtDynWorld->getBroadphase();

	// NB: ~CMyDiscreteDynamicsWorld() deletes remaining objects in the world
	delete pBtDynWorld;
	delete pBtSolver;
	delete pBtCollDisp;
	delete pBtCollCfg;
	delete pBtBroadPhase;
}
//---------------------------------------------------------------------

void CPhysicsLevel::Update(float dt)
{
	ZoneScoped;

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

// FIXME CONSISTENCY: contact enumerators work only with CPhysicsObject but GetClosestRayContact detects hit point for any bullet collision objects!
// NB: may return true when the PPhysicsObject is nullptr! pExclude is optional.
bool CPhysicsLevel::GetClosestRayContact(const rtm::vector4f& Start, const rtm::vector4f& End, U32 Group, U32 Mask, rtm::vector4f* pOutPos, PPhysicsObject* pOutObj, CPhysicsObject* pExclude) const
{
	if (!pBtDynWorld) FAIL;

	const btVector3 BtStart = Math::ToBullet3(Start);
	const btVector3 BtEnd = Math::ToBullet3(End);

	CClosestRayResultCallbackWithExclude RayCB(BtStart, BtEnd, pExclude);
	RayCB.m_collisionFilterGroup = Group;
	RayCB.m_collisionFilterMask = Mask;
	pBtDynWorld->rayTest(BtStart, BtEnd, RayCB);

	if (!RayCB.hasHit()) FAIL;

	if (pOutPos) *pOutPos = Math::FromBullet(RayCB.m_hitPointWorld);
	if (pOutObj) *pOutObj = RayCB.m_collisionObject ? static_cast<CPhysicsObject*>(RayCB.m_collisionObject->getUserPointer()) : nullptr;

	OK;
}
//---------------------------------------------------------------------

//???struct CContact instead of separate object + pos? May add normal.
UPTR CPhysicsLevel::EnumRayContacts(const rtm::vector4f& Start, const rtm::vector4f& End, U32 Group, U32 Mask, std::function<bool(CPhysicsObject&, const rtm::vector4f&)>&& Callback) const
{
	if (!pBtDynWorld) return 0;

	const btVector3 BtStart = Math::ToBullet3(Start);
	const btVector3 BtEnd = Math::ToBullet3(End);
	btCollisionWorld::AllHitsRayResultCallback RayCB(BtStart, BtEnd); // FIXME: use enumerating callback instead of an array!
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

UPTR CPhysicsLevel::EnumObjectContacts(const CPhysicsObject& Object, std::function<bool(CPhysicsObject&, const rtm::vector4f&)>&& Callback) const
{
	if (!pBtDynWorld || !Object.GetBtObject()) return 0;

	CFunctorContactCallback CB(*Object.GetBtObject(), Callback);
	pBtDynWorld->contactTest(Object.GetBtObject(), CB);

	//???or instead of object pass precreated shape here, and create ghost/collision object for the test!
	// Collision objects with a callback still have collision response with dynamic rigid bodies.
	// In order to use collision objects as trigger, you have to disable the collision response. 
	//Object.GetBtObject()->setCollisionFlags(Object.GetBtObject()->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE));

	return CB.GetContactCount();
}
//---------------------------------------------------------------------

UPTR CPhysicsLevel::EnumSphereContacts(const rtm::vector4f& Position, float Radius, U32 Group, U32 Mask, std::function<bool(CPhysicsObject&, const rtm::vector4f&)>&& Callback) const
{
	if (!pBtDynWorld || !Callback || Radius <= 0.f) return 0;

	btSphereShape BtShape(Radius);
	btCollisionObject BtObject;
	BtObject.setCollisionShape(&BtShape);
	BtObject.getWorldTransform().setOrigin(Math::ToBullet3(Position));

	CFunctorContactCallback CB(BtObject, Group, Mask, Callback);
	pBtDynWorld->contactTest(&BtObject, CB);

	return CB.GetContactCount();
}
//---------------------------------------------------------------------

UPTR CPhysicsLevel::EnumCapsuleYContacts(const rtm::vector4f& Position, float Radius, float CylinderLength, U32 Group, U32 Mask, std::function<bool(CPhysicsObject&, const rtm::vector4f&)>&& Callback) const
{
	if (!pBtDynWorld || !Callback) return 0;

	if (CylinderLength <= 0.f) return EnumSphereContacts(Position, Radius, Group, Mask, std::move(Callback));

	btCapsuleShape BtShape(Radius, CylinderLength);
	btCollisionObject BtObject;
	BtObject.setCollisionShape(&BtShape);
	BtObject.getWorldTransform().setOrigin(Math::ToBullet3(Position));

	CFunctorContactCallback CB(BtObject, Group, Mask, Callback);
	pBtDynWorld->contactTest(&BtObject, CB);

	return CB.GetContactCount();
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
	pBtDynWorld->setGravity(Math::ToBullet3(Math::ToSIMD(NewGravity)));
}
//---------------------------------------------------------------------

void CPhysicsLevel::GetGravity(vector3& Out) const
{
	n_assert(pBtDynWorld);
	Out = Math::FromSIMD3(Math::FromBullet(pBtDynWorld->getGravity()));
}
//---------------------------------------------------------------------

}
