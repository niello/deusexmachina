#include "Interaction.h"
#include <Game/Interaction/InteractionContext.h>

namespace DEM::Game
{
CInteraction::~CInteraction() = default;

bool CInteraction::IsCandidateTargetValid(const CGameSession& Session, const CInteractionContext& Context) const
{
	const size_t CurrTargetCount = Context.Targets.size();
	const size_t MaxTargetCount = _MandatoryTargets + _OptionalTargets;
	return (CurrTargetCount < MaxTargetCount) && IsTargetValid(Session, CurrTargetCount, Context);
}
//---------------------------------------------------------------------

bool CInteraction::AreSelectedTargetsValid(const CGameSession& Session, const CInteractionContext& Context) const
{
	for (U32 i = 0; i < Context.Targets.size(); ++i)
		if (!IsTargetValid(Session, i, Context)) return false;
	return true;
}
//---------------------------------------------------------------------

}
