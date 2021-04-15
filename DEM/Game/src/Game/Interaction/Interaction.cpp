#include "Interaction.h"
#include <Game/Interaction/TargetFilter.h>
#include <Game/Interaction/InteractionContext.h>

namespace DEM::Game
{
CInteraction::~CInteraction() = default;

// FIXME: reimplements for new targeting system!
U32 CInteraction::GetMaxTargetCount() const
{
	//U32 Count = 0;
	//for (const auto& Rec : _Targets)
	//	Count += Rec.Count;
	//return Count;
	return 1;
}
//---------------------------------------------------------------------

bool CInteraction::IsCandidateTargetValid(const CGameSession& Session, const CInteractionContext& Context) const
{
	return IsTargetValid(Session, Context.SelectedTargetCount, Context);
}
//---------------------------------------------------------------------

bool CInteraction::AreSelectedTargetsValid(const CGameSession& Session, const CInteractionContext& Context) const
{
	for (U32 i = 0; i < Context.SelectedTargetCount; ++i)
		if (!IsTargetValid(Session, i, Context)) return false;
	return true;
}
//---------------------------------------------------------------------

// TODO: cursor image based on interaction ctx?! E.g. different for different targets.
const std::string& CInteraction::GetCursorImageID(U32 Index) const
{
	return _CursorImage;
}
//---------------------------------------------------------------------

}
