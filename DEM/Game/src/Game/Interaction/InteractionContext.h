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
class IInteraction;
class CTarget;

struct CTargetInfo
{
	HEntity            Entity;
	Scene::CSceneNode* pNode = nullptr;
	vector3            Point;
	bool               Valid = false;
};

class CInteractionContext
{
public:

	CAbility*             pAbility = nullptr;
	IInteraction*         pInteraction = nullptr;
	std::vector<HEntity>  SelectedActors;
	std::vector<CTarget*> SelectedTargets;
	UPTR                  SelectedTargetCount = 0;

	//???store prev frame info? store time we are targeted at this object? for tooltip.
	CTargetInfo           Target;

	bool IsSelectedActor(HEntity ID) const { return std::find(SelectedActors.cbegin(), SelectedActors.cend(), ID) != SelectedActors.cend(); }

	//???where to process input? under mouse info collection, per-frame update/validation
	//of current point and already selected targets, input event listening.
	// DoAction, EnqueueAction =>
	// SelectTarget (LMB), SelectTargetNoReset (Shift+LMB), RevertLast (Esc), ConfirmPartialAction (Enter)
};

}
