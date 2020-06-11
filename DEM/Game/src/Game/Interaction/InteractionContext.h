#pragma once
#include <Game/ECS/Entity.h>
#include <Math/Vector3.h>

namespace Scene
{
	class CSceneNode;
}

namespace DEM::Game
{
class CTarget;

struct CTargetInfo
{
	HEntity            Entity;
	Scene::CSceneNode* pNode = nullptr;
	vector3            Point;
	bool               Valid = false;
};

struct CInteractionContext
{
	CStrID                Ability; //???need also ability source? for items! HEntity or what?
	CStrID                Interaction;
	std::vector<HEntity>  SelectedActors;
	std::vector<CTarget*> SelectedTargets;
	UPTR                  SelectedTargetCount = 0;

	//???store prev frame info? store time we are targeted at this object? for tooltip.
	CTargetInfo           Target;

	bool IsSelectedActor(HEntity ID) const { return std::find(SelectedActors.cbegin(), SelectedActors.cend(), ID) != SelectedActors.cend(); }
	bool AreAllTargetsSet() const { return !SelectedTargets.empty() && SelectedTargets.size() == SelectedTargetCount; }

	//???where to process input? under mouse info collection, per-frame update/validation
	//of current point and already selected targets, input event listening.
	// SelectTarget (LMB), SelectTargetNoReset (Shift+LMB), RevertLast (Esc), ConfirmPartialAction (Enter)
};

}
