#include "ChoiceAction.h"
#include <Conversation/ConversationManager.h>
#include <Conversation/PhraseAction.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Scripting/Flow/FlowAsset.h>
#include <Scripting/Flow/HubAction.h>
#include <Core/Factory.h>

namespace DEM::RPG
{
FACTORY_CLASS_IMPL(DEM::RPG::CChoiceAction, 'CHOA', Flow::IFlowAction);

static const CStrID sidSpeaker("Speaker");
static const CStrID sidText("Text");

void CChoiceAction::CollectChoicesFromLink(CChoiceAction& Root, const Flow::CFlowLink& Link, Game::CGameSession& Session)
{
	const auto* pActionData = Root._pPlayer->GetAsset()->FindAction(Link.DestID);
	if (!pActionData) return;
	const auto* pLinkedRTTI = Core::CFactory::Instance().GetRTTI(pActionData->ClassName.CStr());

	if (CPhraseAction::RTTI.IsBaseOf(pLinkedRTTI))
	{
		// Skip answers of invalid speakers
		auto* pWorld = Session.FindFeature<Game::CGameWorld>();
		if (!pWorld || !CanSpeak(*pWorld, Flow::ResolveEntityID(*pActionData, sidSpeaker, Root._pPlayer->GetVars()))) return;

		// Skip already reached answers
		const auto It = std::find_if(Root._ChoiceLinks.cbegin(), Root._ChoiceLinks.cend(), [ID = Link.DestID](const auto* pLink) { return pLink->DestID == ID; });
		if (It != Root._ChoiceLinks.cend()) return;

		Root._ChoiceTexts.push_back(pActionData->Params->Get<CString>(sidText, CString::Empty).CStr());
		Root._ChoiceLinks.push_back(&Link);
	}
	else if (CChoiceAction::RTTI.IsBaseOf(pLinkedRTTI))
	{
		// Collect recursively. This is useful for grouping choices under the same condition.
		CollectChoices(Root, *pActionData, Session);
	}
	else if (Flow::CHubAction::RTTI.IsBaseOf(pLinkedRTTI))
	{
		// Process hubs transparently as if we executed through them using their rules
		if (const auto* pNextLink = Flow::CHubAction::ChooseNext(*pActionData, Session, Root._pPlayer->GetVars(), Root._pPlayer->GetRNG()))
			CollectChoicesFromLink(Root, *pNextLink, Session);
	}
	// Can add more supported types here
}
//---------------------------------------------------------------------

void CChoiceAction::CollectChoices(CChoiceAction& Root, const Flow::CFlowActionData& Curr, Game::CGameSession& Session)
{
	ForEachValidLink(Curr, Session, Root._pPlayer->GetVars(), [&Root, &Session](size_t Index, const Flow::CFlowLink& Link)
	{
		CollectChoicesFromLink(Root, Link, Session);
	});
}
//---------------------------------------------------------------------

void CChoiceAction::OnStart(Game::CGameSession& Session)
{
	_Speaker = ResolveEntityID(sidSpeaker);
	_ChoiceMadeConn = {};
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
