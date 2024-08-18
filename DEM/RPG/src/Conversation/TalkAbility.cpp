#include "TalkAbility.h"
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/Zone.h>
#include <Game/ECS/GameWorld.h>
#include <Character/StatsComponent.h>
#include <Conversation/TalkingComponent.h>
#include <Conversation/ConversationManager.h>

namespace DEM::RPG
{

CTalkAbility::CTalkAbility(std::string_view CursorImage)
{
	_Name = "Talk";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

bool CTalkAbility::IsAvailable(const Game::CGameSession& Session, const Game::CInteractionContext& Context) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	for (auto ActorID : Context.Actors)
		if (auto pStats = pWorld->FindComponent<const Sh2::CStatsComponent>(ActorID))
			if (pStats->Capabilities & Sh2::ECapability::Talk) return true;

	return false;
}
//---------------------------------------------------------------------

bool CTalkAbility::IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const
{
	// Ability accepts only one target
	if (Index != 0) return false;

	// Can speak with itself
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid || (!Context.Actors.empty() && Target.Entity == Context.Actors[0])) return false;

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	// Need a conversation asset assigned
	auto* pTalking = pWorld->FindComponent<const CTalkingComponent>(Target.Entity);
	return pTalking && pTalking->Asset;
}
//---------------------------------------------------------------------

bool CTalkAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	// Push standard action for the first actor only
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	return pWorld && PushStandardExecuteAction(*pWorld, Context.Actors[0], Context, Enqueue, PushChild);
}
//---------------------------------------------------------------------

bool CTalkAbility::GetZones(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, std::vector<const Game::CZone*>& Out) const
{
	// If smart object provided zones, don't add a default one
	if (!Out.empty()) return true;

	//!!!FIXME: need some way to add zones! Store per-ability? May need unique zones. Use strong refs, not raw pointers?
	static Game::CZone Zone(rtm::vector_zero(), 2.f);
	Out.push_back(&Zone);
	return true;
}
//---------------------------------------------------------------------

bool CTalkAbility::GetFacingParams(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, Game::CFacingParams& Out) const
{
	// Simply face an object origin
	//???make this logic default???
	Out.Mode = Game::EFacingMode::Point;
	Out.Dir = vector3::Zero;
	Out.Tolerance = 5.f;
	return true;
}
//---------------------------------------------------------------------

void CTalkAbility::OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pConvMgr = Session.FindFeature<CConversationManager>();
	if (!pConvMgr) return;

	pConvMgr->DisengageParticipant(Instance.Targets[0].Entity);
	pConvMgr->StartConversation(Instance.Actor, Instance.Targets[0].Entity);
}
//---------------------------------------------------------------------

Game::EActionStatus CTalkAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pConvMgr = Session.FindFeature<CConversationManager>();
	if (!pConvMgr) return Game::EActionStatus::Cancelled;

	//???fail if can't find a conversation?! how to process StartConversation failure?!
	//???enable failures from CAbility::OnStart? return bool and turn false into EActionStatus::Failed in a calling system.
	//???or leave as now and continue checking GetConversationKey to know if dlg has ended. But is it failed start or correct finish?!

	//!!!TODO: catch conversation end event from manager!!!

	return pConvMgr->GetConversationKey(Instance.Actor) ? Game::EActionStatus::Active : Game::EActionStatus::Succeeded;
}
//---------------------------------------------------------------------

void CTalkAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, Game::EActionStatus Status) const
{
	auto pConvMgr = Session.FindFeature<CConversationManager>();
	if (!pConvMgr) return;

	pConvMgr->CancelConversation(Instance.Actor);
}
//---------------------------------------------------------------------

}
