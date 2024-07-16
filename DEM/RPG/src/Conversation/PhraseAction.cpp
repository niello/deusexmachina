#include "PhraseAction.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::RPG
{
FACTORY_CLASS_IMPL(DEM::RPG::CPhraseAction, 'PHRA', Flow::IFlowAction);

static const CStrID sidSpeaker("Speaker");
static const CStrID sidText("Text");

void CPhraseAction::OnStart()
{
	_Speaker = {};

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
	//!!!show phrase in fg or bg and then wait for time or for signal!
	if (auto* pParam = _pPrototype->Params->Find(sidText))
		if (auto& Text = pParam->GetValue<CString>())
			::Sys::DbgOut((Game::EntityToString(_Speaker) + ": " + Text.CStr()).c_str());
}
//---------------------------------------------------------------------

}
