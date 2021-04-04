#pragma once
#include <StdDEM.h>

// Base class for all player->world and character->world interactions.
// Each interaction accepts one or more targets, some of which may be optional.
// Interaction knows when it is available and owns interaction logic.

namespace DEM::Game
{
using PTargetFilter = std::unique_ptr<class ITargetFilter>;
struct CInteractionContext;

class CInteraction
{
protected:

	struct CTargetRecord
	{
		PTargetFilter Filter; //???TODO: use C++/Lua callable instead?! ITargetFilter is just a functor.
		std::string   CursorImage;
		U32           Count;
		bool          IsOptional; // NB: can have mandatory and optional targets in mixed order (really need?)
	};

	std::vector<CTargetRecord> _Targets;

	std::string                _Name;
	std::string                _CursorImage;
	//???icon?

	const CTargetRecord* GetTargetRecord(U32 Index) const;

public:

	virtual ~CInteraction();

	//!!!FIXME: target count may depend on actor/ctx!!! E.g. fire rays, one ray per 2 willpower! Virtualize target related methods?
	bool AddTarget(PTargetFilter&& Filter, std::string_view CursorImage = {}, U32 Count = 1, bool IsOptional = false);

	U32                  GetMaxTargetCount() const;
	const ITargetFilter* GetTargetFilter(U32 Index) const;
	bool                 IsTargetOptional(U32 Index) const;

	const auto&          GetName() const { return _Name; }
	const std::string&   GetCursorImageID(U32 Index) const;

	virtual bool         IsAvailable(const CInteractionContext& Context) const { return true; }
	virtual bool         Execute(CInteractionContext& Context, bool Enqueue) const = 0;
};

}
