#pragma once
#include <Game/GameSession.h>
#include <Game/ECS/Entity.h>
#include <Math/Vector3.h>

namespace Scene
{
	class CSceneNode;
}

namespace DEM::Game
{
using PGameSession = Ptr<class CGameSession>;

struct CTargetInfo
{
	Scene::CSceneNode* pNode = nullptr;
	vector3            Point;
	HEntity            Entity;
	bool               Valid = false;
};

struct CInteractionContext
{
	static inline constexpr auto NO_INTERACTION = std::numeric_limits<U32>().max();

	PGameSession             Session;
	CStrID                   Ability;
	HEntity                  AbilitySource; // E.g. item
	U32                      InteractionIndex = NO_INTERACTION;
	CTargetInfo              Target;
	std::vector<HEntity>     SelectedActors;
	std::vector<CTargetInfo> SelectedTargets;
	U32                      SelectedTargetCount = 0;

	bool IsInteractionSet() const { return InteractionIndex != NO_INTERACTION; }
	bool AreAllTargetsSet() const { return !SelectedTargets.empty() && SelectedTargets.size() == SelectedTargetCount; }
};

}
