#include "SmartObjectLoader.h"
#include <Game/Objects/SmartObject.h>
#include <Resources/ResourceManager.h>
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

	CStrID DefaultState = Params.Get(CStrID("DefaultState"), CStrID::Empty);
	const auto Script = Params.Get(CStrID("Script"), CString::Empty);

	DEM::Game::PSmartObject SmartObject = n_new(DEM::Game::CSmartObject(DefaultState, Script.CStr()));

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
				//!!!defaults! when speed is negative, invert start and end? (from 1 to 0)
				//???validate right here?
				//Task.Timeline = _ResMgr.RegisterResource<DEM::Anim::CTimelineTrack>(TimelineID.CStr());
				//Task.Speed;
				//Task.StartTime;
				//Task.EndTime;
				//Task.LoopCount;
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
						// ...
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
