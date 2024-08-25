#include "PhraseAction.h"
#include <Conversation/ConversationManager.h>
#include <Game/GameSession.h>
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::RPG
{
FACTORY_CLASS_IMPL(DEM::RPG::CPhraseAction, 'PHRA', Flow::IFlowAction);

static const CStrID sidSpeaker("Speaker");
static const CStrID sidText("Text");
static const CStrID sidTime("Time");
static const CStrID sidConversationOwner("ConversationOwner");

// Check that the phrase is said from the same conversation to which an actor is bound, including 'no conversation' case
bool CPhraseAction::IsMatchingConversation(Game::HEntity Speaker, Game::CGameSession& Session, const Flow::CFlowVarStorage& Vars)
{
	auto* pConvMgr = Session.FindFeature<CConversationManager>();
	if (!pConvMgr) return false;

	const int Raw = Vars.Get<int>(Vars.Find(sidConversationOwner), static_cast<int>(Game::HEntity{}.Raw));
	const Game::HEntity ConversationOwner{ static_cast<DEM::Game::HEntity::TRawValue>(Raw) };
	const auto EngagedConvKey = pConvMgr->GetConversationKey(Speaker);
	return EngagedConvKey == ConversationOwner;
}
//---------------------------------------------------------------------

void CPhraseAction::OnStart(Game::CGameSession& Session)
{
	_Speaker = ResolveEntityID(sidSpeaker);
	_PhraseEndConn = {};
	_State = EState::Created;
}
//---------------------------------------------------------------------

void CPhraseAction::Update(Flow::CUpdateContext& Ctx)
{
	if (_State == EState::Created)
	{
		std::string_view TextStr = _pPrototype->Params->Get<CString>(sidText, CString::Empty);
		if (TextStr.empty()) return Break(Ctx);

		// Set this before SayPhrase call because it can set finished state immediately!
		_State = EState::Started;

		// NB: _PhraseEndConn unsubscribes in destructor so capturing raw 'this' is safe here as long as views don't store the callback in an unsafe way
		auto* pConvMgr = Ctx.pSession->FindFeature<CConversationManager>();
		if (pConvMgr && IsMatchingConversation(_Speaker, *Ctx.pSession, _pPlayer->GetVars()))
		{
			const float Time = _pPrototype->Params->Get<float>(sidTime, -1.f);
			_PhraseEndConn = pConvMgr->SayPhrase(_Speaker, std::string(TextStr), Time, [this](bool Ok) { _State = Ok ? EState::Finished : EState::Error; });
		}
		else
		{
			_State = EState::Finished;
		}
	}
	else if (_State == EState::Started)
	{
		if (!IsMatchingConversation(_Speaker, *Ctx.pSession, _pPlayer->GetVars()))
			_State = EState::Finished;
	}

	if (_State == EState::Finished)
		Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
	else if (_State == EState::Error)
		Throw(Ctx, "CPhraseAction - Conversation manager failure", false);
}
//---------------------------------------------------------------------

}
