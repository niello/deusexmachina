#include "SmartObjectLoader.h"
#include <Game/Objects/SmartObject.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Math/SIMDMath.h>

namespace Resources
{

const DEM::Core::CRTTI& CSmartObjectLoader::GetResultType() const
{
	return DEM::Game::CSmartObject::RTTI;
}
//---------------------------------------------------------------------

DEM::Core::PObject CSmartObjectLoader::CreateResource(CStrID UID)
{
	// TODO: can support optional multiple templates in one file through sub-ID
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	// Read params from resource HRD
	Data::CParams Params;
	Data::CHRDParser Parser;
	if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), Params)) return nullptr;

	const CStrID ID = Params.Get(CStrID("ID"), CStrID::Empty);
	const CStrID DefaultState = Params.Get(CStrID("DefaultState"), CStrID::Empty);
	const CStrID ScriptAssetID = Params.Get(CStrID("Script"), CStrID::Empty);

	// Load object states and transitions
	std::vector<DEM::Game::CSmartObjectStateInfo> States;
	Data::PParams StatesDesc;
	if (Params.TryGet<Data::PParams>(StatesDesc, CStrID("States")))
	{
		States.reserve(StatesDesc->GetCount());
		for (const auto& StateParam : *StatesDesc)
		{
			const auto& StateDesc = *StateParam.GetValue<Data::PParams>();

			// Optional timeline player task
			DEM::Anim::CTimelineTask Task;
			Data::PParams TimelineTaskDesc;
			if (StateDesc.TryGet<Data::PParams>(TimelineTaskDesc, CStrID("Timeline")) && TimelineTaskDesc)
				DEM::Anim::InitTimelineTask(Task, *TimelineTaskDesc, _ResMgr);

			DEM::Game::CSmartObjectStateInfo StateInfo{ StateParam.GetName(), std::move(Task) };

			// Transitions from this state
			//???!!!for integrity validation can load all states, then all transitions?!
			Data::PParams TransitionsDesc;
			if (StateDesc.TryGet<Data::PParams>(TransitionsDesc, CStrID("Transitions")))
			{
				StateInfo.Transitions.reserve(TransitionsDesc->GetCount());
				for (const auto& TransitionParam : *TransitionsDesc)
				{
					const auto& TransitionDesc = *TransitionParam.GetValue<Data::PParams>();

					DEM::Anim::CTimelineTask Task;
					Data::PParams TimelineTaskDesc;
					if (TransitionDesc.TryGet<Data::PParams>(TimelineTaskDesc, CStrID("Timeline")) && TimelineTaskDesc)
					{
						DEM::Anim::InitTimelineTask(Task, *TimelineTaskDesc, _ResMgr);

						// Transition must always be finite
						if (!Task.LoopCount) Task.LoopCount = 1;
					}

					DEM::Game::ETransitionInterruptionMode InterruptionMode = DEM::Game::ETransitionInterruptionMode::ResetToStart;
					const auto& ModeStr = TransitionDesc.Get(CStrID("InterruptionMode"), CString::Empty);
					if (ModeStr == "ResetToStart") InterruptionMode = DEM::Game::ETransitionInterruptionMode::ResetToStart;
					else if (ModeStr == "RewindToEnd") InterruptionMode = DEM::Game::ETransitionInterruptionMode::RewindToEnd;
					else if (ModeStr == "Proportional") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Proportional;
					else if (ModeStr == "Forbid") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Forbid;
					else if (ModeStr == "Force") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Force;
					else if (ModeStr == "Wait") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Wait;
					else if (ModeStr.IsValid()) ::Sys::Error("CSmartObjectLoader::CreateResource() > Unknown transition interruption mode");

					StateInfo.Transitions.push_back({ TransitionParam.GetName(), std::move(Task), InterruptionMode });
				}
			}

			States.push_back(std::move(StateInfo));
		}
	}

	// Load interaction zones and available interactions
	std::vector<DEM::Game::CZone> Zones;
	if (auto* pParam = Params.Find(CStrID("Zones")))
		DEM::ParamsFormat::Deserialize(pParam->GetRawValue(), Zones);

	// Load interaction override rules
	std::map<CStrID, CFixedArray<CStrID>> Overrides;
	Data::PParams OverridesDesc;
	if (Params.TryGet<Data::PParams>(OverridesDesc, CStrID("Overrides")))
	{
		for (const auto& OverrideParam : *OverridesDesc)
		{
			const auto& IDsDesc = *OverrideParam.GetValue<Data::PDataArray>();
			if (IDsDesc.empty()) continue;

			CFixedArray<CStrID> IDs(IDsDesc.size());
			for (UPTR i = 0; i < IDsDesc.size(); ++i)
				IDs[i] = IDsDesc[i].GetValue<CStrID>();

			Overrides.emplace(OverrideParam.GetName(), std::move(IDs));
		}
	}

	return n_new(DEM::Game::CSmartObject(ID, DefaultState, ScriptAssetID, std::move(States), std::move(Zones), std::move(Overrides)));
}
//---------------------------------------------------------------------

}
