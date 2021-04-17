#include "Interaction.h"
#include <Game/Interaction/InteractionContext.h>

namespace DEM::Game
{
CInteraction::~CInteraction() = default;

bool CInteraction::IsCandidateTargetValid(const CGameSession& Session, const CInteractionContext& Context) const
{
	return IsTargetValid(Session, Context.Targets.size(), Context);
}
//---------------------------------------------------------------------

bool CInteraction::AreSelectedTargetsValid(const CGameSession& Session, const CInteractionContext& Context) const
{
	for (U32 i = 0; i < Context.Targets.size(); ++i)
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
