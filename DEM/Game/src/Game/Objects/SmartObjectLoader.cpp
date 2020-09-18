#include "SmartObjectLoader.h"
#include <Game/Objects/SmartObject.h>
#include <Resources/ResourceManager.h>
#include <Animation/TimelineTrack.h>
#include <IO/Stream.h>
#include <Data/Params.h>
#include <Data/Buffer.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>

namespace Resources
{

const Core::CRTTI& CSmartObjectLoader::GetResultType() const
{
	return DEM::Game::CSmartObject::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CSmartObjectLoader::CreateResource(CStrID UID)
{
	// TODO: can support optional multiple templates in one file through sub-ID
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID, pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	// Read params from resource HRD
	Data::CParams Params;
	Data::CHRDParser Parser;
	if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), Params)) return nullptr;

	// Read script source
	const auto ScriptPath = Params.Get(CStrID("Script"), CString::Empty);
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(ScriptPath, pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
		if (!Buffer) return nullptr;
	}
	std::string_view ScriptSource(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize());

	const CStrID ID = Params.Get(CStrID("ID"), CStrID::Empty);
	const CStrID DefaultState = Params.Get(CStrID("DefaultState"), CStrID::Empty);

	DEM::Game::PSmartObject SmartObject = n_new(DEM::Game::CSmartObject(ID, DefaultState, ScriptSource));

	Data::PParams StatesDesc;
	if (Params.TryGet<Data::PParams>(StatesDesc, CStrID("States")))
	{
		for (const auto& StateParam : *StatesDesc)
		{
			const auto& StateDesc = *StateParam.GetValue<Data::PParams>();

			// Optional timeline player task
			DEM::Game::CTimelineTask Task;
			if (const CStrID TimelineID = StateDesc.Get(CStrID("Timeline"), CStrID::Empty))
			{
				Task.Timeline = _ResMgr.RegisterResource<DEM::Anim::CTimelineTrack>(TimelineID.CStr());
				Task.Speed = StateDesc.Get(CStrID("Speed"), 1.f);
				Task.StartTime = StateDesc.Get(CStrID("StartTime"), (Task.Speed > 0.f) ? 0.f : 1.f);
				Task.EndTime = StateDesc.Get(CStrID("EndTime"), (Task.Speed > 0.f) ? 1.f : 0.f);
				Task.LoopCount = StateDesc.Get(CStrID("LoopCount"), 0);

				Task.Timeline->ValidateObject();
			}

			SmartObject->AddState(StateParam.GetName(), std::move(Task));

			// Transitions from this state
			//???!!!for integrity validation can load states, then transitions?!
			Data::PParams TransitionsDesc;
			if (StateDesc.TryGet<Data::PParams>(TransitionsDesc, CStrID("Transitions")))
			{
				for (const auto& TransitionParam : *TransitionsDesc)
				{
					const auto& TransitionDesc = *TransitionParam.GetValue<Data::PParams>();

					DEM::Game::CTimelineTask Task;
					if (const CStrID TimelineID = TransitionDesc.Get(CStrID("Timeline"), CStrID::Empty))
					{
						Task.Timeline = _ResMgr.RegisterResource<DEM::Anim::CTimelineTrack>(TimelineID.CStr());
						Task.Speed = StateDesc.Get(CStrID("Speed"), 1.f);
						Task.StartTime = StateDesc.Get(CStrID("StartTime"), (Task.Speed > 0.f) ? 0.f : 1.f);
						Task.EndTime = StateDesc.Get(CStrID("EndTime"), (Task.Speed > 0.f) ? 1.f : 0.f);
						Task.LoopCount = StateDesc.Get(CStrID("LoopCount"), 0);

						Task.Timeline->ValidateObject();
					}

					SmartObject->AddTransition(StateParam.GetName(), TransitionParam.GetName(), std::move(Task));
				}
			}
		}
	}

	Data::PDataArray Actions;
	if (Params.TryGet<Data::PDataArray>(Actions, CStrID("Actions")))
		for (const auto& ActionData : *Actions)
			if (auto pActionStr = ActionData.As<CStrID>())
				SmartObject->AddInteraction(*pActionStr);

	return SmartObject;
}
//---------------------------------------------------------------------

}
