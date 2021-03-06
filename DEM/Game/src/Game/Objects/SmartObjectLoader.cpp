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

Core::PObject CSmartObjectLoader::CreateResource(CStrID UID)
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
					//else if (ModeStr == "Force") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Force;
					//else if (ModeStr == "Wait") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Wait;
					else if (ModeStr.IsValid()) ::Sys::Error("CSmartObjectLoader::CreateResource() > Unknown transition interruption mode");

					StateInfo.Transitions.push_back({ TransitionParam.GetName(), std::move(Task), InterruptionMode });
				}
			}

			States.push_back(std::move(StateInfo));
		}
	}

	// Load interaction zones and available interactions
	std::vector<DEM::Game::CZone> Zones;
	Data::PDataArray ZoneDescs;
	if (Params.TryGet<Data::PDataArray>(ZoneDescs, CStrID("Zones")))
	{
		Zones.reserve(ZoneDescs->GetCount());
		for (const auto& ZoneParam : *ZoneDescs)
		{
			const auto& ZoneDesc = *ZoneParam.GetValue<Data::PParams>();

			DEM::Game::CZone Zone;
			Zone.Radius = ZoneDesc.Get(CStrID("Radius"), 0.f);

			Data::PDataArray VerticesDesc;
			if (ZoneDesc.TryGet<Data::PDataArray>(VerticesDesc, CStrID("Vertices")))
			{
				Zone.Vertices.SetSize(VerticesDesc->GetCount());
				size_t i = 0;
				for (const auto& VertexDesc : *VerticesDesc)
					Zone.Vertices[i++] = VertexDesc.GetValue<vector3>();

				if (Zone.Vertices.size())
				{
					Zone.ClosedPolygon = ZoneDesc.Get(CStrID("ClosedPoly"), false);
					// TODO: Zone.ConvexPolygon = ZoneDesc.Get(CStrID("ConvexPoly"), false);
					//???or detect from points?
				}
			}

			Zones.push_back(std::move(Zone));
		}
	}

	// Load interaction override rules
	std::map<CStrID, CFixedArray<CStrID>> Overrides;
	Data::PParams OverridesDesc;
	if (Params.TryGet<Data::PParams>(OverridesDesc, CStrID("Overrides")))
	{
		for (const auto& OverrideParam : *OverridesDesc)
		{
			const auto& IDsDesc = *OverrideParam.GetValue<Data::PDataArray>();
			if (IDsDesc.IsEmpty()) continue;

			CFixedArray<CStrID> IDs(IDsDesc.GetCount());
			for (UPTR i = 0; i < IDsDesc.GetCount(); ++i)
				IDs[i] = IDsDesc[i].GetValue<CStrID>();

			Overrides.emplace(OverrideParam.GetName(), std::move(IDs));
		}
	}

	return n_new(DEM::Game::CSmartObject(ID, DefaultState, ScriptSource, std::move(States), std::move(Zones), std::move(Overrides)));
}
//---------------------------------------------------------------------

}
