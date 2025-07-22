#include "TimelineTask.h"
#include <Animation/Timeline/PoseTrack.h>
#include <Animation/Timeline/TimelinePlayer.h>
#include <Animation/Skeleton.h>
#include <Game/ECS/GameWorld.h>
#include <Scene/SceneComponent.h>

namespace DEM::Anim
{

void InitTimelineTask(CTimelineTask& Task, const Data::CParams& Desc, Resources::CResourceManager& ResMgr)
{
	if (const CStrID TimelineID = Desc.Get(CStrID("Asset"), CStrID::Empty))
	{
		Task.Timeline = ResMgr.RegisterResource<CTimelineTrack>(TimelineID.CStr());
		Task.Timeline->ValidateObject();
	}
	else Task.Timeline = nullptr;

	Task.Speed = Desc.Get(CStrID("Speed"), 1.f);
	Task.StartTime = Desc.Get(CStrID("StartTime"), 0.f);
	Task.EndTime = Desc.Get(CStrID("EndTime"), 1.f);
	Task.LoopCount = Desc.Get(CStrID("LoopCount"), 0);
	Task.OutputDescs = Desc.Get(CStrID("Outputs"), Data::PParams{});
}

// TODO: add context for passing in arbitrary params, like a smart object user ID?
bool LoadTimelineTaskIntoPlayer(CTimelinePlayer& Player, Game::CGameWorld& World, Game::HEntity Owner,
	const CTimelineTask& Task, const CTimelineTask* pPrevTask)
{
	if (!Task.Timeline)
	{
		Player.SetTrack(nullptr);
		return false;
	}

	const auto* pNewTrackProto = Task.Timeline->GetObject<Anim::CTimelineTrack>();
	if (!pNewTrackProto)
	{
		Player.SetTrack(nullptr);
		return false;
	}

	// If current track is cloned from the same prototype, can reuse it and avoid heavy setup
	// FIXME: track output recreation may be required if output context changed!
	const auto* pOldTrackProto = pPrevTask ? pPrevTask->Timeline->GetObject<Anim::CTimelineTrack>() : nullptr;
	if (!Player.GetTrack() || pNewTrackProto != pOldTrackProto)
	{
		Anim::PTimelineTrack NewTrack = pNewTrackProto->Clone();

		// Setup outputs for tracks recursively
		// FIXME: grouping tracks through a composite pattern looks strange now. Maybe this is a wrong design decision.
		NewTrack->Visit([&Task, &World, Owner](Anim::CTimelineTrack& Track)
		{
			Data::CData* pOutputDesc;
			if (!Task.OutputDescs || !Task.OutputDescs->TryGet(pOutputDesc, Track.GetName())) return;

			if (auto pPoseTrack = Track.As<Anim::CPoseTrack>())
			{
				// TODO: Player.GetTrack()->FindTrackByName(pPoseTrack->GetName());
				// If found, may reuse output CSkeleton if skeleton info is the same?

				if (!pPoseTrack->GetSkeletonInfo() || !pOutputDesc->IsA<std::string>()) return;

				if (auto pSceneComponent = World.FindComponent<Game::CSceneComponent>(Owner))
				{
					const std::string& SkeletonRootPath = pOutputDesc->GetValue<std::string>();
					if (auto pTargetRoot = pSceneComponent->RootNode->FindNodeByPath(SkeletonRootPath.c_str()))
					{
						Anim::PSkeleton Skeleton(n_new(Anim::CSkeleton()));
						Skeleton->Init(*pTargetRoot, *pPoseTrack->GetSkeletonInfo());
						pPoseTrack->SetOutput(std::move(Skeleton));
					}
				}
			}
		});

		Player.SetTrack(NewTrack);
	}

	//???always relative time? may need absolute, especially when editing single multisegment TL.
	const float TrackDuration = Player.GetTrack()->GetDuration();
	Player.SetStartTime(Task.StartTime * TrackDuration);
	Player.SetEndTime(Task.EndTime * TrackDuration);
	Player.SetSpeed(Task.Speed);
	return true;
}

}
