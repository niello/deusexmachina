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
		PTargetFilter Filter;
		std::string   CursorImage;
		U32           Count;
		bool          Optional; // NB: can have mandatory target after optional
	};

	std::vector<CTargetRecord> _Targets;

	std::string                _Name;
	std::string                _CursorImage;

	const CTargetRecord* GetTargetRecord(U32 Index) const;

public:

	// AddTarget - or private and only in CScriptableInteraction?

	U32                  GetMaxTargetCount() const;
	const ITargetFilter* GetTargetFilter(U32 Index) const;
	bool                 IsTargetOptional(U32 Index) const;

	const auto&          GetName() const { return _Name; }
	const std::string&   GetCursorImageID(U32 Index) const; //!!!if target one is empty, return from this class

	virtual bool         Execute(CInteractionContext& Context, bool Enqueue) const = 0;
};

}
