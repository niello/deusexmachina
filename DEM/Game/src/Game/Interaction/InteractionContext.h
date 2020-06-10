#pragma once
#include <Game/ECS/Entity.h>
#include <Math/Vector3.h>

namespace Scene
{
	class CSceneNode;
}

namespace DEM::Game
{
class CAbility;
class CAction;
class CTarget;

class CInteractionContext
{
public:

	CAbility*             pAbility = nullptr;
	CAction*              pAction = nullptr;
	std::vector<HEntity>  SelectedActors;
	std::vector<CTarget*> SelectedTargets;
	UPTR                  SelectedTargetCount = 0;

	//???store prev frame intersection? store time over this object? for tooltip.
	HEntity               EntityUnderCursor;
	Scene::CSceneNode*    pNodeUnderCursor = nullptr;
	vector3               PointUnderCursor;
	bool                  HasWorldUnderCursor = false;

	bool IsSelectedActor(HEntity ID) const { return std::find(SelectedActors.cbegin(), SelectedActors.cend(), ID) != SelectedActors.cend(); }

	//???where to process input? under mouse info collection, per-frame update/validation
	//of current point and already selected targets, input event listening.
	// DoAction, EnqueueAction =>
	// SelectTarget (LMB), SelectTargetNoReset (Shift+LMB), RevertLast (Esc), ConfirmPartialAction (Enter)
};

}
