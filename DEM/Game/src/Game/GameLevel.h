#pragma once
#include <Data/RefCounted.h>
#include <Game/ECS/Entity.h>
#include <Frame/GraphicsScene.h>

// Represents one game location. Consists of subsystem worlds (scene, graphics, physics, AI).
// In MVC pattern it is a model.

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace Physics
{
	typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
	class CPhysicsObject;
}

namespace DEM::AI
{
	using PAILevel = Ptr<class CAILevel>;
	using PNavMap = Ptr<class CNavMap>;
}

namespace Resources
{
	class CResourceManager;
	typedef Ptr<class CResource> PResource;
}

namespace Data
{
	class CParams;
}

namespace DEM::Game
{
typedef Ptr<class CGameLevel> PGameLevel;

class CGameLevel : public Data::CRefCounted
{
protected:

	CStrID                 _ID;

	Scene::PSceneNode      _SceneRoot;
	Frame::CGraphicsScene  _GraphicsScene;
	Physics::PPhysicsLevel _PhysicsLevel;
	AI::PAILevel           _AILevel;

	std::vector<DEM::AI::PNavMap> _NavMaps; // Sorted by R & H

public:

	static PGameLevel LoadFromDesc(CStrID ID, const Data::CParams& In, Resources::CResourceManager& ResMgr);

	CGameLevel(CStrID ID, const Math::CAABB& Bounds, const Math::CAABB& InteractiveBounds = Math::EmptyAABB(), UPTR SubdivisionDepth = 0);
	virtual ~CGameLevel() override;

	bool                     Validate(Resources::CResourceManager& RsrcMgr);
	void                     Update(float dt, const rtm::vector4f* pCOIArray, UPTR COICount);

	void                     SetNavRegionController(CStrID RegionID, HEntity Controller);
	void                     SetNavRegionFlags(CStrID RegionID, U16 Flags, bool On);

	Physics::CPhysicsObject* GetFirstPickIntersection(const rtm::vector4f& RayFrom, const rtm::vector4f& RayTo, rtm::vector4f* pOutPoint3D = nullptr, std::string_view CollisionMask = {}, HEntity ExcludeID = {}) const;
	// Query hierarchy:
	// 4. Reachable entities in a shape (navigation)
	// 5. Reachable entities in a shape filtered by a custom filter, e.g. by a component presence
	//ray check may need to apply collision group filter after the raycast, e.g. when searching for
	//the closest Interactable, if the user wants closer non-interactable collision objects to block it.
	void                     EnumEntitiesInSphere(const rtm::vector4f& Position, float Radius, std::string_view CollisionMask, std::function<bool(HEntity&, const rtm::vector4f&)>&& Callback) const;

	template<typename TPredicate>
	Game::HEntity FindClosestEntity(const rtm::vector4f& Position, float Radius, std::string_view CollisionMask, TPredicate Predicate = nullptr)
	{
		float ClosestDistanceSq = std::numeric_limits<float>().max();
		Game::HEntity ClosestEntityID;

		EnumEntitiesInSphere(Position, Radius, CollisionMask,
			[&ClosestDistanceSq, &ClosestEntityID, Position, Predicate](Game::HEntity EntityID, const rtm::vector4f& Pos)
			{
				const float DistanceSq = Math::vector_distance_squared3(Pos, Position);
				if (ClosestDistanceSq <= DistanceSq) return true;

				if constexpr (!std::is_same_v<TPredicate, std::nullptr_t>)
					if (!Predicate(EntityID)) return true;

				ClosestDistanceSq = DistanceSq;
				ClosestEntityID = EntityID;
				return true;
			});

		return ClosestEntityID;
	}

	CStrID                   GetID() const { return _ID; }

	Scene::CSceneNode&       GetSceneRoot() { return *_SceneRoot.Get(); }
	Frame::CGraphicsScene&   GetGraphics() { return _GraphicsScene; }
	Physics::CPhysicsLevel*  GetPhysics() const { return _PhysicsLevel.Get(); }
	AI::CAILevel*            GetAI() const { return _AILevel.Get(); }
	AI::CNavMap*             GetNavMap(float AgentRadius, float AgentHeight) const;
};

}
