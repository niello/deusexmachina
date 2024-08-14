#include "DisengageFromConversationAction.h"
#include <Conversation/ConversationManager.h>
#include <Game/GameSession.h>
#include <Core/Factory.h>

namespace DEM::RPG
{
FACTORY_CLASS_IMPL(DEM::RPG::CDisengageFromConversationAction, 'DFCA', Flow::IFlowAction);

static const CStrID sidActor("Actor");
static const CStrID sidConversationOwner("ConversationOwner");

void CDisengageFromConversationAction::OnStart(Game::CGameSession& Session)
{
	_Actor = ResolveEntityID(sidActor);
}
//---------------------------------------------------------------------

void CDisengageFromConversationAction::Update(Flow::CUpdateContext& Ctx)
{
	if (auto pConvMgr = Ctx.pSession->FindFeature<CConversationManager>())
	{
		const int Raw = _pPlayer->GetVars().Get<int>(_pPlayer->GetVars().Find(sidConversationOwner), static_cast<int>(Game::HEntity{}.Raw));
		Game::HEntity ConversationOwner{ static_cast<DEM::Game::HEntity::TRawValue>(Raw) };
		if (ConversationOwner)
		{
			const bool Mandatory = pConvMgr->IsParticipantMandatory(_Actor);
			if (pConvMgr->DisengageParticipant(ConversationOwner, _Actor))
				return Mandatory ? Break(Ctx) : Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
		}
	}

	Throw(Ctx, "Failed to engage actor in conversation", false);
}
//---------------------------------------------------------------------

}
