#include "ChoiceAction.h"
#include <Conversation/ConversationManager.h>
#include <Conversation/PhraseAction.h>
#include <Game/GameSession.h>
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::RPG
{
FACTORY_CLASS_IMPL(DEM::RPG::CChoiceAction, 'CHOA', Flow::IFlowAction);

static const CStrID sidSpeaker("Speaker");
static const CStrID sidText("Text");

void CChoiceAction::CollectChoices(CChoiceAction& Root, const Flow::CFlowActionData& Curr, const Game::CGameSession& Session)
{
	ForEachValidLink(Curr, Session, Root._pPlayer->GetVars(), [&Root, &Session](size_t Index, const Flow::CFlowLink& Link)
	{
		const auto* pActionData = Root._pPlayer->GetAsset()->FindAction(Link.DestID);
		if (!pActionData) return;
		const auto* pLinkedRTTI = Core::CFactory::Instance().GetRTTI(pActionData->ClassName.CStr());

		if (CPhraseAction::RTTI.IsBaseOf(pLinkedRTTI))
		{
			Root._ChoiceTexts.push_back(pActionData->Params->Get<CString>(sidText, CString::Empty).CStr());
			Root._ChoiceLinks.push_back(&Link);
		}
		else if (CChoiceAction::RTTI.IsBaseOf(pLinkedRTTI))
		{
			// Collect recursively. This is useful for grouping choices under the same condition.
			CollectChoices(Root, *pActionData, Session);
		}
	});
}
//---------------------------------------------------------------------

void CChoiceAction::OnStart(Game::CGameSession& Session)
{
	_Speaker = {};
	_ChoiceMadeConn = {};

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

	// Collect answers
	CollectChoices(*this, *_pPrototype, Session);
	_Choice = _ChoiceLinks.size();
}
//---------------------------------------------------------------------

void CChoiceAction::Update(Flow::CUpdateContext& Ctx)
{
	if (!_ChoiceMadeConn)
	{
		if (_ChoiceTexts.empty()) return Break(Ctx);

		// NB: _ChoiceMadeConn unsubscribes in destructor so capturing raw 'this' is safe here as long as views don't store the callback in an unsafe way
		if (auto pConvMgr = Ctx.pSession->FindFeature<CConversationManager>())
			_ChoiceMadeConn = pConvMgr->ProvideChoices(_Speaker, std::move(_ChoiceTexts), [this](size_t Index) { _Choice = Index; });
		else
			return Break(Ctx);
	}

	// TODO: can add processing for special _Choice values, e.g. forced break!
	if (_Choice < _ChoiceLinks.size())
		Goto(Ctx, _ChoiceLinks[_Choice]);
}
//---------------------------------------------------------------------

}