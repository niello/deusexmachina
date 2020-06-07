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

class CActionContext
{
public:

	CAbility*            pAbility = nullptr;
	std::vector<HEntity> Selection;
	// target slots (filled and not)
	// number of filled slots?

	//???store prev frame intersection? store time over this object? for tooltip.
	HEntity              EntityUnderCursor;
	Scene::CSceneNode*   pNodeUnderCursor = nullptr;
	vector3              PointUnderCursor;
	bool                 HasSomethingUnderCursor = false;

	bool IsSelectedActor(HEntity ID) const { return std::find(Selection.cbegin(), Selection.cend(), ID) != Selection.cend(); }

	//???where to process input? under mouse info collection, per-frame update/validation
	//of current point and already selected targets, input event listening.
	// DoAction, EnqueueAction =>
	// SelectTarget (LMB), SelectTargetNoReset (Shift+LMB), RevertLast (Esc), ConfirmPartialAction (Enter)
};

}
