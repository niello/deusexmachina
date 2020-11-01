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
	const bool Static = Params.Get(CStrID("Static"), false);

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
			DEM::Game::CTimelineTask Task;
			if (const CStrID TimelineID = StateDesc.Get(CStrID("Timeline"), CStrID::Empty))
			{
				Task.Timeline = _ResMgr.RegisterResource<DEM::Anim::CTimelineTrack>(TimelineID.CStr());
				Task.Speed = StateDesc.Get(CStrID("Speed"), 1.f);
				Task.StartTime = StateDesc.Get(CStrID("StartTime"), 0.f);
				Task.EndTime = StateDesc.Get(CStrID("EndTime"), 1.f);
				Task.LoopCount = StateDesc.Get(CStrID("LoopCount"), 0); // State is infinite by default
				Task.SkeletonRootRelPath = StateDesc.Get(CStrID("SkeletonRootRelPath"), CString::Empty).CStr();

				Task.Timeline->ValidateObject();
			}

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

					DEM::Game::CTimelineTask Task;
					if (const CStrID TimelineID = TransitionDesc.Get(CStrID("Timeline"), CStrID::Empty))
					{
						Task.Timeline = _ResMgr.RegisterResource<DEM::Anim::CTimelineTrack>(TimelineID.CStr());
						Task.Speed = TransitionDesc.Get(CStrID("Speed"), 1.f);
						Task.StartTime = TransitionDesc.Get(CStrID("StartTime"), 0.f);
						Task.EndTime = TransitionDesc.Get(CStrID("EndTime"), 1.f);
						Task.LoopCount = TransitionDesc.Get(CStrID("LoopCount"), 1); // Transition must always be finite
						Task.SkeletonRootRelPath = TransitionDesc.Get(CStrID("SkeletonRootRelPath"), CString::Empty).CStr();

						Task.Timeline->ValidateObject();
					}

					DEM::Game::ETransitionInterruptionMode InterruptionMode = DEM::Game::ETransitionInterruptionMode::ResetToStart;
					const auto& ModeStr = TransitionDesc.Get(CStrID("InterruptionMode"), CString::Empty);
					if (ModeStr == "ResetToStart") InterruptionMode = DEM::Game::ETransitionInterruptionMode::ResetToStart;
					else if (ModeStr == "RewindToEnd") InterruptionMode = DEM::Game::ETransitionInterruptionMode::RewindToEnd;
					else if (ModeStr == "Proportional") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Proportional;
					else if (ModeStr == "Forbid") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Forbid;
					//else if (ModeStr == "Force") InterruptionMode = DEM::Game::ETransitionInterruptionMode::Force;
					else if (ModeStr.IsValid()) ::Sys::Error("CSmartObjectLoader::CreateResource() > Unknown interruption mode");

					StateInfo.Transitions.push_back({ TransitionParam.GetName(), std::move(Task), InterruptionMode });
				}
			}

			States.push_back(std::move(StateInfo));
		}
	}

	// Load interaction zones and available interactions
	std::vector<DEM::Game::CInteractionZone> InteractionZones;
	Data::PDataArray ZoneDescs;
	if (Params.TryGet<Data::PDataArray>(ZoneDescs, CStrID("Interactions")))
	{
		InteractionZones.reserve(ZoneDescs->GetCount());
		for (const auto& ZoneParam : *ZoneDescs)
		{
			const auto& ZoneDesc = *ZoneParam.GetValue<Data::PParams>();

			DEM::Game::CInteractionZone Zone;
			Zone.Radius = ZoneDesc.Get(CStrID("Radius"), 0.f);

			Data::PDataArray VerticesDesc;
			if (Params.TryGet<Data::PDataArray>(VerticesDesc, CStrID("Vertices")))
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

			// Interactions available in this zone
			Data::PParams ActionsDesc;
			if (ZoneDesc.TryGet<Data::PParams>(ActionsDesc, CStrID("Actions")))
			{
				Zone.Interactions.SetSize(ActionsDesc->GetCount());
				size_t i = 0;
				for (const auto& ActionParam : *ActionsDesc)
				{
					auto& Interaction = Zone.Interactions[i++];
					Interaction.ID = ActionParam.GetName();

					const auto& ActionDesc = *ActionParam.GetValue<Data::PParams>();
					// FacingMode, FacingDir, ActorAnim etc
				}
			}

			InteractionZones.push_back(std::move(Zone));
		}
	}


	return n_new(DEM::Game::CSmartObject(ID, DefaultState, Static, ScriptSource, std::move(States), std::move(InteractionZones)));
}
//---------------------------------------------------------------------

}
