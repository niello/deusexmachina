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
	PGameSession             Session;
	CStrID                   Ability;
	HEntity                  AbilitySource; // E.g. item
	CStrID                   Interaction;
	sol::function            Condition;
	CTargetInfo              Target;
	std::vector<HEntity>     SelectedActors;
	std::vector<CTargetInfo> SelectedTargets;
	U32                      SelectedTargetCount = 0;

	bool AreAllTargetsSet() const { return !SelectedTargets.empty() && SelectedTargets.size() == SelectedTargetCount; }
};

}
