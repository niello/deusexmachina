#pragma once
#include <StdDEM.h>

// Base class for all player->world and character->world interactions.
// Each interaction accepts one or more targets, some of which may be optional.
// Interaction knows when it is available and owns interaction logic.

// FIXME: interactions are per-session, they may store sol::function etc. Remove session from API, get inside?
// TODO: optional target - implement through target validation? For optional return true if no target selected?

namespace DEM::Game
{
using PInteraction = std::unique_ptr<class CInteraction>;
using PTargetFilter = std::unique_ptr<class ITargetFilter>;
struct CInteractionContext;
class CGameSession;

class CInteraction
{
protected:

	std::string                _Name;
	std::string                _CursorImage;
	//???icon?

public:

	virtual ~CInteraction();

	// FIXME: variable, use validation instead? ExpectsTarget()?
	U32                  GetMaxTargetCount() const;

	bool                 IsCandidateTargetValid(const CGameSession& Session, const CInteractionContext& Context) const;
	bool                 AreSelectedTargetsValid(const CGameSession& Session, const CInteractionContext& Context) const;

	const auto&          GetName() const { return _Name; }
	const std::string&   GetCursorImageID(U32 Index) const;

	virtual bool         IsAvailable(const CInteractionContext& Context) const { return true; }
	virtual bool         IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const = 0;
	virtual bool         Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue) const = 0;
};

}
