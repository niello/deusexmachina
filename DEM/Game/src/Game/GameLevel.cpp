#include "GameLevel.h"
#include <Game/Interaction/InteractionContext.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/LightAttribute.h>
#include <Scene/SceneNode.h>
#include <Scene/NodeAttribute.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/PhysicsObject.h>
#include <Physics/CollisionAttribute.h>
#include <Physics/BulletConv.h>
#include <AI/Navigation/NavMap.h>
#include <AI/AILevel.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Data/DataArray.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>

namespace DEM::Game
{
bool GetTargetFromPhysicsObject(const Physics::CPhysicsObject& Object, CTargetInfo& OutTarget);

CGameLevel::CGameLevel(CStrID ID, const Math::CAABB& Bounds, const Math::CAABB& InteractiveBounds, UPTR SubdivisionDepth)
	: _ID(ID)
	, _SceneRoot(n_new(Scene::CSceneNode(ID)))
	, _PhysicsLevel(n_new(Physics::CPhysicsLevel(Bounds)))
	, _AILevel(n_new(AI::CAILevel()))
{
	const auto BoundsSize = Math::FromSIMD3(rtm::vector_mul(Bounds.Extent, 2.f));
	_GraphicsScene.Init(Bounds.Center, std::max({ BoundsSize.x, BoundsSize.y, BoundsSize.z }), SubdivisionDepth ? SubdivisionDepth : 12);
}
//---------------------------------------------------------------------

CGameLevel::~CGameLevel()
{
	// Order of destruction is important
	_SceneRoot = nullptr;
	_PhysicsLevel = nullptr;
}
//---------------------------------------------------------------------

PGameLevel CGameLevel::LoadFromDesc(CStrID ID, const Data::CParams& In, Resources::CResourceManager& ResMgr)
{
	vector3 Center(vector3::Zero);
	vector3 Size(512.f, 128.f, 512.f);
	int SubdivisionDepth = 0;

	In.TryGet(Center, CStrID("Center"));
	In.TryGet(Size, CStrID("Size"));
	In.TryGet(SubdivisionDepth, CStrID("SubdivisionDepth"));
	vector3 InteractiveCenter = In.Get(CStrID("InteractiveCenter"), Center);
	vector3 InteractiveExtents = In.Get(CStrID("InteractiveSize"), Size) * 0.5f;
	vector3 Extents = Size * 0.5f;

	PGameLevel Level = new CGameLevel(
		ID,
		Math::CAABB{ Math::ToSIMD(Center), Math::ToSIMD(Extents) },
		Math::CAABB{ Math::ToSIMD(InteractiveCenter), Math::ToSIMD(InteractiveExtents) },
		SubdivisionDepth);

	// Load optional scene with static graphics, collision and other attributes. No entity is associated with it.
	const bool StaticSceneIsUnique = In.Get(CStrID("StaticSceneIsUnique"), true);
	if (auto StaticScene = In.Get(CStrID("StaticScene"), Data::PParams()))
	{
		for (const auto& Param : *StaticScene)
		{
			// This resource can be unloaded by the client code when reloading it in the near future is not expected.
			// The most practical way is to check resources with refcount = 1, they are held by a resource manager only.
			// Use StaticSceneIsUnique = false if you expect to use the scene in multuple level instances and you
			// plan to modify it in the runtime (which is not recommended nor typical for _static_ scenes).
			auto Rsrc = ResMgr.RegisterResource<Scene::CSceneNode>(Param.GetValue<std::string>().c_str());
			if (auto StaticSceneNode = Rsrc->ValidateObject<Scene::CSceneNode>())
			{
				// If no reuse allowed, ensure it or fall back to shared resource
				// Rsrc is referenced here and in a resource manager.
				// StaticSceneNode is referenced inside Rsrc, raw pointer is used here.
				if (StaticSceneIsUnique && Rsrc->GetRefCount() <= 2 && StaticSceneNode->GetRefCount() <= 1)
				{
					// Unregister unique scene from resources to prevent unintended reuse which can cause huge problems
					Level->GetSceneRoot().AddChild(Param.GetName(), StaticSceneNode);
					ResMgr.UnregisterResource(Rsrc->GetUID());
				}
				else
				{
					Level->GetSceneRoot().AddChild(Param.GetName(), StaticSceneNode->Clone());
				}
			}
		}
	}

	// Load navigation meshes, if present
	if (auto Navigation = In.Get(CStrID("Navigation"), Data::PDataArray()))
	{
		for (const auto& Elm : *Navigation)
		{
			const auto& NavDesc = Elm.GetValue<Data::PParams>();
			const float AgentRadius = NavDesc->Get(CStrID("AgentRadius"), -0.f);
			const float AgentHeight = NavDesc->Get(CStrID("AgentHeight"), -0.f);
			const auto NavigationMapID = NavDesc->Get(CStrID("NavMesh"), EmptyString);
			auto Rsrc = ResMgr.RegisterResource<DEM::AI::CNavMesh>(NavigationMapID.c_str());
			if (AgentRadius <= 0.f || AgentHeight <= 0.f || !Rsrc) continue;

			if (NavDesc->Get(CStrID("Preload"), false))
				Rsrc->ValidateObject<DEM::AI::CNavMesh>();

			Level->_NavMaps.push_back(n_new(AI::CNavMap)(AgentRadius, AgentHeight, Rsrc));
		}

		std::sort(Level->_NavMaps.begin(), Level->_NavMaps.end(),
			[](const AI::PNavMap& a, const AI::PNavMap& b)
		{
			// Sort by radius, then by height ascending
			return a->GetAgentRadius() < b->GetAgentRadius() ||
				(a->GetAgentRadius() == b->GetAgentRadius() && a->GetAgentHeight() < b->GetAgentHeight());
		});
	}

	return std::move(Level);
}
//---------------------------------------------------------------------

bool CGameLevel::Validate(Resources::CResourceManager& RsrcMgr)
{
	// force entities to spawn into worlds (scene, physics etc)

	const bool Result = _SceneRoot->Visit([&RsrcMgr](Scene::CSceneNode& Node)
	{
		for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
			if (!Node.GetAttribute(i)->ValidateResources(RsrcMgr)) return false;
		return true;
	});
	if (!Result) FAIL;

	if (_PhysicsLevel)
	{
		if (!_SceneRoot->Visit([PhysicsLevel = _PhysicsLevel](Scene::CSceneNode& Node)
		{
			for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
				if (auto pAttrTyped = Node.GetAttribute(i)->As<Physics::CCollisionAttribute>())
					pAttrTyped->SetPhysicsLevel(PhysicsLevel);
			OK;
		}
		)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CGameLevel::Update(float dt, const rtm::vector4f* pCOIArray, UPTR COICount)
{
	ZoneScoped;

	if (_PhysicsLevel) _PhysicsLevel->Update(dt);

	{
		ZoneScopedN("Scene hierarchy update");

		_SceneRoot->Update(pCOIArray, COICount);
	}

	{
		ZoneScopedN("Renderable & light transform update");

		// TODO: build some list?
		_SceneRoot->Visit([this](Scene::CSceneNode& Node)
		{
			for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
			{
				Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
				if (!Attr.IsActive()) continue;

				if (auto pAttr = Attr.As<Frame::CRenderableAttribute>())
					pAttr->UpdateInGraphicsScene(_GraphicsScene);
				else if (auto pAttr = Attr.As<Frame::CLightAttribute>())
					pAttr->UpdateInGraphicsScene(_GraphicsScene);
			}

			OK;
		});
	}
}
//---------------------------------------------------------------------

Physics::CPhysicsObject* CGameLevel::GetFirstPickIntersection(const rtm::vector4f& RayFrom, const rtm::vector4f& RayTo, rtm::vector4f* pOutPoint3D, std::string_view CollisionMask, HEntity ExcludeID) const
{
	ZoneScoped;

	if (!_PhysicsLevel) return nullptr;

	auto* pBtWorld = _PhysicsLevel->GetBtWorld();
	if (!pBtWorld) return nullptr;

	class CEntityExcludingRayCallback : public btCollisionWorld::ClosestRayResultCallback
	{
	protected:

		HEntity _ExcludeID;

	public:

		CEntityExcludingRayCallback(const btVector3& From, const btVector3& To, HEntity ExcludeID = {})
			: ClosestRayResultCallback(From, To)
			,_ExcludeID(ExcludeID)
		{
		}

		virtual bool needsCollision(btBroadphaseProxy* proxy0) const override
		{
			if (!(proxy0->m_collisionFilterGroup & m_collisionFilterMask)) return false;
			if (!(m_collisionFilterGroup & proxy0->m_collisionFilterMask)) return false;

			// TODO: if guaranteed to receive each object once, could remember if already met excluded entity and stop checking for it
			if (_ExcludeID)
			{
				if (const auto* pPhysObj = static_cast<Physics::CPhysicsObject*>(static_cast<btCollisionObject*>(proxy0->m_clientObject)->getUserPointer()))
				{
					CTargetInfo Target;
					GetTargetFromPhysicsObject(*pPhysObj, Target);
					if (Target.Entity == _ExcludeID) return false;
				}
			}

			return true;
		}
	};

	const btVector3 BtStart = Math::ToBullet3(RayFrom);
	const btVector3 BtEnd = Math::ToBullet3(RayTo);

	CEntityExcludingRayCallback RayCB(BtStart, BtEnd, ExcludeID);
	RayCB.m_collisionFilterGroup = _PhysicsLevel->PredefinedCollisionGroups.Query;
	RayCB.m_collisionFilterMask = CollisionMask.empty() ? _PhysicsLevel->PredefinedCollisionGroups.All : _PhysicsLevel->CollisionGroups.GetMask(CollisionMask);
	pBtWorld->rayTest(BtStart, BtEnd, RayCB);

	if (!RayCB.hasHit()) return nullptr;

	if (pOutPoint3D) *pOutPoint3D = Math::FromBullet(RayCB.m_hitPointWorld);
	return RayCB.m_collisionObject ? static_cast<Physics::CPhysicsObject*>(RayCB.m_collisionObject->getUserPointer()) : nullptr;
}
//---------------------------------------------------------------------

void CGameLevel::EnumEntitiesInSphere(const rtm::vector4f& Position, float Radius, std::string_view CollisionMask, std::function<bool(HEntity&, const rtm::vector4f&)>&& Callback) const
{
	if (!_PhysicsLevel || !Callback || Radius <= 0.f) return;

	auto* pBtWorld = _PhysicsLevel->GetBtWorld();
	if (!pBtWorld) return;

	// Optimized to skip physics objects without DEM entities bound. Skips costly tests against static level geometry.
	struct CEntityOnlyContactCallback : public btCollisionWorld::ContactResultCallback
	{
		btCollisionObject&                                   _Self;
		std::function<bool(HEntity&, const rtm::vector4f&)>& _Callback;

		CEntityOnlyContactCallback(btCollisionObject& Self, std::function<bool(HEntity&, const rtm::vector4f&)>& Callback)
			: _Self(Self), _Callback(Callback)
		{
		}

		virtual bool needsCollision(btBroadphaseProxy* proxy0) const override
		{
			if (!(proxy0->m_collisionFilterGroup & m_collisionFilterMask)) return false;
			if (!(m_collisionFilterGroup & proxy0->m_collisionFilterMask)) return false;

			const auto* pPhysObj = static_cast<Physics::CPhysicsObject*>(static_cast<btCollisionObject*>(proxy0->m_clientObject)->getUserPointer());
			if (!pPhysObj) return false;

			CTargetInfo Target;
			GetTargetFromPhysicsObject(*pPhysObj, Target);
			return !!Target.Entity;
		}

		virtual btScalar addSingleResult(btManifoldPoint& cp,
			const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
			const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
		{
			// NB: the same object can be reported multiple times
			const bool IsMeFirst = (colObj0Wrap->getCollisionObject() == &_Self);
			const auto* pBtObj = IsMeFirst ? colObj1Wrap->getCollisionObject() : colObj0Wrap->getCollisionObject();
			const auto* pPhysObj = static_cast<Physics::CPhysicsObject*>(pBtObj->getUserPointer());
			CTargetInfo Target;
			GetTargetFromPhysicsObject(*pPhysObj, Target);
			_Callback(Target.Entity, Math::FromBullet(IsMeFirst ? cp.m_positionWorldOnB : cp.m_positionWorldOnA));
			return 0;
		}
	};

	btSphereShape BtShape(Radius);
	btCollisionObject BtObject;
	BtObject.setCollisionShape(&BtShape);
	BtObject.getWorldTransform().setOrigin(Math::ToBullet3(Position));

	CEntityOnlyContactCallback CB(BtObject, Callback);
	CB.m_collisionFilterGroup = _PhysicsLevel->PredefinedCollisionGroups.Query;
	CB.m_collisionFilterMask = CollisionMask.empty() ? _PhysicsLevel->PredefinedCollisionGroups.All : _PhysicsLevel->CollisionGroups.GetMask(CollisionMask);
	pBtWorld->contactTest(&BtObject, CB);
}
//---------------------------------------------------------------------

DEM::AI::CNavMap* CGameLevel::GetNavMap(float AgentRadius, float AgentHeight) const
{
	// Navigation meshes are sorted by agent radius, then by height, so the first
	// matching navmesh is the best one as it allows the most possible movement.
	auto It = _NavMaps.begin();
	for (; It != _NavMaps.end(); ++It)
		if (AgentRadius <= (*It)->GetAgentRadius() && AgentHeight <= (*It)->GetAgentHeight())
			break;

	return (It != _NavMaps.end()) ? (*It) : nullptr;
}
//---------------------------------------------------------------------

void CGameLevel::SetNavRegionController(CStrID RegionID, HEntity Controller)
{
	for (const auto& NavMap : _NavMaps)
		NavMap->SetRegionController(RegionID, Controller);
}
//---------------------------------------------------------------------

void CGameLevel::SetNavRegionFlags(CStrID RegionID, U16 Flags, bool On)
{
	for (const auto& NavMap : _NavMaps)
		NavMap->SetRegionFlags(RegionID, Flags, On);
}
//---------------------------------------------------------------------

}
