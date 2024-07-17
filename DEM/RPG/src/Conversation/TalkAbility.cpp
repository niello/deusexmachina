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
		if (auto pStats = pWorld->FindComponent<Sh2::CStatsComponent>(ActorID))
			if (pStats->Capabilities & Sh2::ECapability::Talk) return true;

	return false;
}
//---------------------------------------------------------------------

bool CTalkAbility::IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const
{
	// Ability accepts only one target
	if (Index != 0) return false;

	// Check for the item stack component
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid) return false;
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	return pWorld && pWorld->FindComponent<const CTalkingComponent>(Target.Entity);
}
//---------------------------------------------------------------------

bool CTalkAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	Game::HEntity ActorID = Context.Actors[0];
	// TODO: choose the first actor with enough inventory space? Or no need?
	//if (Context.Actors.size() > 1)
	//{
	//}

	// Push standard action for the first actor only
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	return pWorld && PushStandardExecuteAction(*pWorld, ActorID, Context, Enqueue, PushChild);
}
//---------------------------------------------------------------------

bool CTalkAbility::GetZones(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, std::vector<const Game::CZone*>& Out) const
{
	// If smart object provided zones, don't add a default one
	if (!Out.empty()) return true;

	//!!!FIXME: need some way to add zones! Store per-ability? May need unique zones. Use strong refs, not raw pointers?
	static Game::CZone Zone(rtm::vector_zero(), 1.f);
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
	Out.Tolerance = 1.5f;
	return true;
}
//---------------------------------------------------------------------

void CTalkAbility::OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pConvMgr = Session.FindFeature<CConversationManager>();
	if (!pConvMgr) return;

	pConvMgr->StartConversation(Instance.Actor, Instance.Targets[0].Entity);
}
//---------------------------------------------------------------------

Game::EActionStatus CTalkAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pConvMgr = Session.FindFeature<CConversationManager>();
	if (!pConvMgr) return Game::EActionStatus::Cancelled;

	//???fail if can't find a conversation?! how to process StartConversation failure?!

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
