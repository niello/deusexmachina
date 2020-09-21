#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/Interaction/Ability.h>
#include <Game/ECS/Entity.h>
#include <map>

// Handles interaction of the player player with the game world. All available
// interactions and abilities available in the current game.
// External systems access them by ID.

namespace Data
{
	class CParams;
}

namespace DEM::Game
{
using PInteraction = std::unique_ptr<class CInteraction>;
struct CInteractionContext;
class CGameSession;

class CInteractionManager : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL;

protected:

	std::map<CStrID, CAbility>     _Abilities;
	std::map<CStrID, PInteraction> _Interactions;
	sol::state_view                _Lua; //???!!!store session ref instead?!
	CStrID                         _DefaultAbility;

	const CInteraction* ValidateInteraction(CStrID ID, const sol::function& Condition, CInteractionContext& Context);

public:

	CInteractionManager(CGameSession& Owner);
	~CInteractionManager();

	bool                RegisterAbility(CStrID ID, CAbility&& Ability);
	bool                RegisterAbility(CStrID ID, const Data::CParams& Params);
	//bool                RegisterAbility(CStrID ID, const CInteraction& SingleInteraction);
	bool                RegisterInteraction(CStrID ID, PInteraction&& Interaction);
	void                SetDefaultAbility(CStrID ID) { _DefaultAbility = ID; }

	const CAbility*     FindAbility(CStrID ID) const;
	const CAbility*     FindAvailableAbility(CStrID AbilityID, const std::vector<HEntity>& SelectedActors) const;
	const CInteraction* FindInteraction(CStrID ID) const;

	bool                SelectAbility(CInteractionContext& Context, CStrID AbilityID, HEntity AbilitySource = {});
	void                ResetAbility(CInteractionContext& Context);
	void                ResetCandidateInteraction(CInteractionContext& Context);
	bool                UpdateCandidateInteraction(CInteractionContext& Context);
	bool                AcceptTarget(CInteractionContext& Context);
	bool                Revert(CInteractionContext& Context);
	bool                ExecuteInteraction(CInteractionContext& Context, bool Enqueue);

	const std::string&  GetCursorImageID(CInteractionContext& Context) const;
};

}
