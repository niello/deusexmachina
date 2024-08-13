#include "EngageInConversationAction.h"
#include <Conversation/ConversationManager.h>
#include <Game/GameSession.h>
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::RPG
{
FACTORY_CLASS_IMPL(DEM::RPG::CEngageInConversationAction, 'EICA', Flow::IFlowAction);

static const CStrID sidActor("Actor");
static const CStrID sidOptional("Optional");
static const CStrID sidConversationOwner("ConversationOwner");

void CEngageInConversationAction::OnStart(Game::CGameSession& Session)
{
	_Actor = ResolveEntityID(sidActor);
	_Optional = _pPrototype->Params->Get<bool>(sidOptional, true);
}
//---------------------------------------------------------------------

void CEngageInConversationAction::Update(Flow::CUpdateContext& Ctx)
{
	if (auto pConvMgr = Ctx.pSession->FindFeature<CConversationManager>())
	{
		const int Raw = _pPlayer->GetVars().Get<int>(_pPlayer->GetVars().Find(sidConversationOwner), static_cast<int>(Game::HEntity{}.Raw));
		Game::HEntity ConversationOwner{ static_cast<DEM::Game::HEntity::TRawValue>(Raw) };
		if ((ConversationOwner && pConvMgr->EngageParticipant(ConversationOwner, _Actor, !_Optional)) || _Optional)
			return Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
	}

	Throw(Ctx, "Failed to engage actor in conversation", false);
}
//---------------------------------------------------------------------

}
