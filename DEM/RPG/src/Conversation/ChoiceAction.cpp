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

void CChoiceAction::CollectChoicesFromLink(CChoiceAction& Root, const Flow::CFlowLink& Link, Game::CGameSession& Session, bool DebugMode, bool IsValid)
{
	const auto* pActionData = Root._pPlayer->GetAsset()->FindAction(Link.DestID);
	if (!pActionData) return;
	const auto* pLinkedRTTI = DEM::Core::CFactory::Instance().GetRTTI(pActionData->ClassName.CStr());

	if (CPhraseAction::RTTI.IsBaseOf(pLinkedRTTI))
	{
		// Skip answers of invalid speakers
		auto* pWorld = Session.FindFeature<Game::CGameWorld>();
		const auto Speaker = Flow::ResolveEntityID(pActionData->Params, sidSpeaker, &Root._pPlayer->GetVars());
		const bool IsPhraseValid = IsValid && pWorld && CanSpeak(*pWorld, Speaker) && CPhraseAction::IsMatchingConversation(Speaker, Session, Root._pPlayer->GetVars());
		if (!IsPhraseValid && !DebugMode) return;

		// Skip already reached answers
		const auto It = std::find_if(Root._ChoiceLinks.cbegin(), Root._ChoiceLinks.cend(), [ID = Link.DestID](const auto* pLink) { return pLink->DestID == ID; });
		if (It != Root._ChoiceLinks.cend())
		{
			if (DebugMode && IsPhraseValid)
			{
				// In case the phrase was recorded as invalid but now reached as valid
				const auto Idx = std::distance(Root._ChoiceLinks.cbegin(), It);
				Root._ChoiceValidFlags[Idx] = true;
			}
			return;
		}

		// TODO: a list of condition types for which text must be displayed in UI? Or text is for UI only? In debug mode show all conditions.
		std::string Text = GetConditionText(Link.Condition, Session, &Root._pPlayer->GetVars());
		if (!Text.empty()) Text.append(": ");
		Text.append(static_cast<std::string_view>(pActionData->Params->Get<CString>(sidText, CString::Empty)));
		Root._ChoiceTexts.push_back(std::move(Text));
		Root._ChoiceLinks.push_back(&Link);
		Root._ChoiceValidFlags.push_back(IsPhraseValid);
	}
	else if (CChoiceAction::RTTI.IsBaseOf(pLinkedRTTI))
	{
		// Collect recursively. This is useful for grouping choices under the same condition.
		CollectChoices(Root, *pActionData, Session, DebugMode, IsValid);
	}
	else if (Flow::CHubAction::RTTI.IsBaseOf(pLinkedRTTI))
	{
		// Process hubs transparently as if we executed through them using their rules
		if (DebugMode)
			CollectChoices(Root, *pActionData, Session, DebugMode, IsValid);
		else if (const auto* pNextLink = Flow::CHubAction::ChooseNext(*pActionData, Session, Root._pPlayer->GetVars(), Root._pPlayer->GetRNG()))
			CollectChoicesFromLink(Root, *pNextLink, Session, DebugMode, IsValid);
	}
	// Can add more supported types here
}
//---------------------------------------------------------------------

void CChoiceAction::CollectChoices(CChoiceAction& Root, const Flow::CFlowActionData& Curr, Game::CGameSession& Session, bool DebugMode, bool IsValid)
{
	if (DebugMode)
	{
		// In debug mode all unavailable answers are listed too
		for (const auto& Link : Curr.Links)
			CollectChoicesFromLink(Root, Link, Session, true, IsValid && EvaluateCondition(Link.Condition, Session, &Root._pPlayer->GetVars()));
	}
	else
	{
		ForEachValidLink(Curr, Session, Root._pPlayer->GetVars(), [&Root, &Session](size_t Index, const Flow::CFlowLink& Link)
		{
			CollectChoicesFromLink(Root, Link, Session, false, true);
		});
	}
}
//---------------------------------------------------------------------

void CChoiceAction::OnStart(Game::CGameSession& Session)
{
	_ChoiceMadeConn = {};

	auto* pConvMgr = Session.FindFeature<CConversationManager>();
	const bool DebugMode = pConvMgr && pConvMgr->IsInDebugMode();

	_ChoiceTexts.clear();
	_ChoiceLinks.clear();
	_ChoiceValidFlags.clear();
	CollectChoices(*this, *_pPrototype, Session, DebugMode, true);
	_Choice = std::nullopt;
}
//---------------------------------------------------------------------

void CChoiceAction::Update(Flow::CUpdateContext& Ctx)
{
	if (!_ChoiceMadeConn)
	{
		if (_ChoiceTexts.empty()) return Break(Ctx);

		// NB: _ChoiceMadeConn unsubscribes in destructor so capturing raw 'this' is safe here as long as views don't store the callback in an unsafe way
		if (auto* pConvMgr = Ctx.pSession->FindFeature<CConversationManager>())
			_ChoiceMadeConn = pConvMgr->ProvideChoices(std::move(_ChoiceTexts), std::move(_ChoiceValidFlags), [this](size_t Index) { _Choice = Index; });
		else
			return Break(Ctx);
	}

	if (_Choice.has_value())
	{
		if (_Choice.value() < _ChoiceLinks.size())
			Goto(Ctx, _ChoiceLinks[_Choice.value()]);
		else
			Break(Ctx);
	}
}
//---------------------------------------------------------------------

}
