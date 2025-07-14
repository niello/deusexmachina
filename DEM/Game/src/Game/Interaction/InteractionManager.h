#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/Interaction/InteractionTool.h>
#include <Game/ECS/Entity.h>
#include <map>

// Handles interaction of the player player with the game world.
// Stores a registry of all possible interactions and interaction tools.
// External systems access interactions and tools by ID.

namespace Data
{
	class CParams;
}

namespace DEM::Game
{
using PInteraction = std::unique_ptr<class CInteraction>;
struct CInteractionContext;
class CGameSession;

class CInteractionManager : public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CInteractionManager, DEM::Core::CRTTIBaseClass);

protected:

	CGameSession&                                    _Session; // Safe because CInteractionManager lives inside a session
	std::map<CStrID, CInteractionTool>               _Tools;
	std::map<CStrID, std::map<CStrID, PInteraction>> _Interactions; // Grouped by smart object ID
	CStrID                                           _DefaultTool;

public:

	CInteractionManager(CGameSession& Owner);
	~CInteractionManager() override;

	bool                    RegisterTool(CStrID ID, CInteractionTool&& Tool);
	bool                    RegisterTool(CStrID ID, const Data::CParams& Params);
	bool                    RegisterInteraction(CStrID ID, PInteraction&& Interaction, CStrID SmartObjectID = {});
	void                    SetDefaultTool(CStrID ID) { _DefaultTool = ID; }

	const CInteractionTool* FindTool(CStrID ID) const;
	const CInteractionTool* FindAvailableTool(CStrID ID, const CInteractionContext& Context) const;
	const CInteraction*     FindInteraction(CStrID ID, CStrID SmartObjectID = {}) const;

	bool                    SelectTool(CInteractionContext& Context, CStrID ToolID, HEntity Source = {}) const;
	void                    ResetTool(CInteractionContext& Context) const;
	void                    ResetCandidateInteraction(CInteractionContext& Context) const;
	bool                    UpdateCandidateInteraction(CInteractionContext& Context) const;
	bool                    AcceptTarget(CInteractionContext& Context) const;
	bool                    AreMandatoryTargetsSelected(CInteractionContext& Context) const;
	bool                    AreMaxTargetsSelected(CInteractionContext& Context) const;
	bool                    Revert(CInteractionContext& Context) const;
	bool                    ExecuteInteraction(CInteractionContext& Context) const;

	const std::string&      GetCursorImageID(CInteractionContext& Context) const;
};

}
