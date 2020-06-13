#pragma once
#include <Game/ECS/Entity.h>
#include <Math/Vector3.h>

namespace Scene
{
	class CSceneNode;
}

namespace DEM::Game
{

struct CTargetInfo
{
	Scene::CSceneNode* pNode = nullptr;
	vector3            Point;
	HEntity            Entity;
	//!!!dtPolyRef NavPolyRef;
	bool               Valid = false;
};

struct CInteractionContext
{
	static inline constexpr auto NO_INTERACTION = std::numeric_limits<U32>().max();

	CStrID                   Ability;
	HEntity                  AbilitySource; // E.g. item
	U32                      InteractionIndex = NO_INTERACTION;
	std::vector<HEntity>     SelectedActors;
	std::vector<CTargetInfo> SelectedTargets;
	U32                      SelectedTargetCount = 0;

	CTargetInfo              Target;

	bool IsSelectedActor(HEntity ID) const { return std::find(SelectedActors.cbegin(), SelectedActors.cend(), ID) != SelectedActors.cend(); }
	bool IsInteractionSet() const { return InteractionIndex != NO_INTERACTION; }
	bool AreAllTargetsSet() const { return !SelectedTargets.empty() && SelectedTargets.size() == SelectedTargetCount; }
};

}
