#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/Interaction/InteractionTool.h>
#include <Game/ECS/Entity.h>
#include <map>

// Handles interaction of the player player with the game world.
// Stores a registry of all possible interactions and interaction tools,
// except interactions defined in smart objects.
// External systems access interactions and tools by ID.

//???register SO interactions here too? Can cache Lua because session is known and Lua context is accessible.

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
	RTTI_CLASS_DECL(CInteractionManager, ::Core::CRTTIBaseClass);

protected:

	std::map<CStrID, CInteractionTool> _Tools;
	std::map<CStrID, PInteraction>     _Interactions;
	sol::state_view                    _Lua; //???!!!store session ref instead?!
	CStrID                             _DefaultTool;

	const CInteraction* ValidateInteraction(CStrID ID, const sol::function& Condition, CInteractionContext& Context) const;

public:

	CInteractionManager(CGameSession& Owner);
	~CInteractionManager();

	bool                    RegisterTool(CStrID ID, CInteractionTool&& Tool);
	bool                    RegisterTool(CStrID ID, const Data::CParams& Params);
	//bool                    RegisterTool(CStrID ID, const CInteraction& SingleInteraction);
	bool                    RegisterInteraction(CStrID ID, PInteraction&& Interaction);
	void                    SetDefaultTool(CStrID ID) { _DefaultTool = ID; }

	const CInteractionTool* FindTool(CStrID ID) const;
	const CInteractionTool* FindAvailableTool(CStrID ID, const CInteractionContext& Context) const;
	const CInteraction*     FindInteraction(CStrID ID) const;

	bool                    SelectTool(CInteractionContext& Context, CStrID ToolID, HEntity Source = {});
	void                    ResetTool(CInteractionContext& Context);
	void                    ResetCandidateInteraction(CInteractionContext& Context);
	bool                    UpdateCandidateInteraction(CInteractionContext& Context);
	bool                    AcceptTarget(CInteractionContext& Context);
	bool                    Revert(CInteractionContext& Context);
	bool                    ExecuteInteraction(CInteractionContext& Context, bool Enqueue);

	const std::string&      GetCursorImageID(CInteractionContext& Context) const;
};

}
