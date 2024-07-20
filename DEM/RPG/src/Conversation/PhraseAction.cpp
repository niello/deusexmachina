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

void CPhraseAction::OnStart()
{
	_Speaker = {};
	_State = EState::Created;

	// TODO: to utility function for reading HEntity in a flow script!
	if (auto* pParam = _pPrototype->Params->Find(sidSpeaker))
	{
		if (pParam->IsA<int>())
		{
			_Speaker = Game::HEntity{ static_cast<DEM::Game::HEntity::TRawValue>(pParam->GetValue<int>()) };
		}
		else if (pParam->IsA<CStrID>())
		{
			const int SpeakerRaw = _pPlayer->GetVars().Get<int>(_pPlayer->GetVars().Find(pParam->GetValue<CStrID>()), static_cast<int>(Game::HEntity{}.Raw));
			_Speaker = Game::HEntity{ static_cast<DEM::Game::HEntity::TRawValue>(SpeakerRaw) };
		}
	}
}
//---------------------------------------------------------------------

void CPhraseAction::Update(Flow::CUpdateContext& Ctx)
{
	if (_State == EState::Created)
	{
		std::string TextStr;
		if (auto* pParam = _pPrototype->Params->Find(sidText))
			if (auto& Text = pParam->GetValue<CString>())
				TextStr = Text.CStr();

		if (TextStr.empty()) return Break(Ctx);

		// Set this before SayPhrase call because it can set finished state immediately!
		_State = EState::Started;

		float Time = -1.f;
		if (auto* pParam = _pPrototype->Params->Find(sidTime))
			Time = pParam->GetValue<float>();

		//!!!TODO: calculate IsLast! must find next phrase or end. Or any linked action is ok?
		bool IsLast = false;

		if (auto pConvMgr = Ctx.pSession->FindFeature<CConversationManager>())
		{
			//!!!!!!FIXME: 'this' is captured by raw value, how to guarantee correctness? Use signal system to auto unsubscribe on destruction?
			//???can use CConnection without CSignal? Just to make self-clearing connection with any user system? Could provide connection for callback.
			//otherwise would have to use shared_ptr+weak_ptr for all flow actions!
			pConvMgr->SayPhrase(_Speaker, std::move(TextStr), IsLast, Time, [this]() { _State = EState::Finished; });
		}
	}

	if (_State == EState::Finished)
	{
		if (_pPrototype->Links.empty())
			Break(Ctx);
		else
			Goto(Ctx, _pPrototype->Links[0]); //!!!GetFirstValidLink()!
	}
}
//---------------------------------------------------------------------

}
