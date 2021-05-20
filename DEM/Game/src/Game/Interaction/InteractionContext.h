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

	CTargetInfo() = default;
	CTargetInfo(HEntity Entity_) : Entity(Entity_), Valid(true) {}
};

struct CInteractionContext
{
	CStrID                   Tool;
	CStrID                   Interaction;
	CTargetInfo              CandidateTarget;
	HEntity                  Source; // E.g. item
	std::vector<HEntity>     Actors;
	std::vector<CTargetInfo> Targets;
	ESoftBool                TargetExpected = ESoftBool::True;
};

}
