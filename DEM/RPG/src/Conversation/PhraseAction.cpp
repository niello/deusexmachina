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
		std::string TextStr = _pPrototype->Params->Get<CString>(sidText, CString::Empty).CStr();
		if (TextStr.empty()) return Break(Ctx);

		// Set this before SayPhrase call because it can set finished state immediately!
		_State = EState::Started;

		const float Time = _pPrototype->Params->Get<float>(sidTime, -1.f);

		// NB: _PhraseEndConn unsubscribes in destructor so capturing raw 'this' is safe here as long as views don't store the callback in an unsafe way
		if (auto pConvMgr = Ctx.pSession->FindFeature<CConversationManager>())
			_PhraseEndConn = pConvMgr->SayPhrase(_Speaker, std::move(TextStr), Time, [this](bool Ok) { _State = Ok ? EState::Finished : EState::Error; });
		else
			_State = EState::Finished;
	}

	if (_State == EState::Finished)
		Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
	else if (_State == EState::Error)
		Throw(Ctx, "CPhraseAction - Conversation manager failure", false);
}
//---------------------------------------------------------------------

}
