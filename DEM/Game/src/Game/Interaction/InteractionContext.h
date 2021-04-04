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

// FIXME: Session not here? pass Session from CInteractionManager along with this ctx? what with Condition then???
struct CInteractionContext
{
	PGameSession             Session;
	CStrID                   Tool;
	CStrID                   Interaction;
	sol::function            Condition;
	CTargetInfo              CandidateTarget;
	HEntity                  Source; // E.g. item
	std::vector<HEntity>     Actors;
	std::vector<CTargetInfo> Targets;
	U32                      SelectedTargetCount = 0; // Required because Targets.size caches max targets for interaction

	bool AreAllTargetsSet() const { return !Targets.empty() && Targets.size() == SelectedTargetCount; }
};

}
