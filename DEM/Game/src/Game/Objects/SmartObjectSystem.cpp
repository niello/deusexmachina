#include <Game/ECS/GameWorld.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>
#include <sol/sol.hpp>

//!!!DBG TMP!
#include <Animation/Skeleton.h>
#include <Animation/PoseTrack.h>
#include <Game/ECS/Components/SceneComponent.h>

namespace DEM::Game
{

static void RunTimelineTask(DEM::Game::CGameWorld& World, HEntity EntityID, CSmartObjectComponent& SOComponent,
	const CTimelineTask& Task, float InitialProgress, Anim::PTimelineTrack CurrTrack, const CTimelineTask* pPrevTask)
{
	n_assert_dbg(InitialProgress >= 0.f && InitialProgress <= 1.f);

	if (!Task.Timeline)
	{
		SOComponent.Player.SetTrack(nullptr);
		return;
	}

	Anim::PTimelineTrack NewTrack;

	// If current track is cloned from the same prototype, can reuse it and avoid heavy setup
	const auto* pOldTrackProto = pPrevTask ? pPrevTask->Timeline->GetObject<Anim::CTimelineTrack>() : nullptr;
	const auto* pNewTrackProto = Task.Timeline->GetObject<Anim::CTimelineTrack>();
	if (!CurrTrack || pNewTrackProto != pOldTrackProto)
	{
		Anim::PTimelineTrack NewTrack = pNewTrackProto->Clone();

		//???!!!DBG TMP!? See Task.SkeletonRootRelPath comment.
		if (auto pPoseTrack = NewTrack->As<Anim::CPoseTrack>())
		{
			auto pOldPoseTrack = CurrTrack ? CurrTrack->As<Anim::CPoseTrack>() : nullptr;
			if (pOldPoseTrack && pPrevTask && pPrevTask->SkeletonRootRelPath == Task.SkeletonRootRelPath)
			{
				// Bone remapping is added per clip where necessary, track itself stores reusable skeleton
				pPoseTrack->SetOutput(pOldPoseTrack->GetOutput());
			}
			else
			{
				if (auto pSceneComponent = World.FindComponent<DEM::Game::CSceneComponent>(EntityID))
					if (auto pTargetRoot = pSceneComponent->RootNode->FindNodeByPath(Task.SkeletonRootRelPath.c_str()))
						pPoseTrack->SetOutput(n_new(Anim::CSkeleton(pTargetRoot)));
			}
		}

		CurrTrack = nullptr;

		SOComponent.Player.SetTrack(NewTrack);
	}
	else SOComponent.Player.SetTrack(CurrTrack);

	//???always relative time? may need absolute, especially when editing single multisegment TL.
	const float TrackDuration = SOComponent.Player.GetTrack()->GetDuration();
	SOComponent.Player.SetStartTime(Task.StartTime * TrackDuration);
	SOComponent.Player.SetEndTime(Task.EndTime * TrackDuration);
	SOComponent.Player.SetSpeed(Task.Speed);

	// Transfer the progress % from the previous transition, if required
	const float FullDuration = SOComponent.Player.GetLoopDuration() * Task.LoopCount;
	SOComponent.Player.SetTime(InitialProgress * FullDuration); //???mul speed sign?

	SOComponent.Player.Play(Task.LoopCount);
}
//---------------------------------------------------------------------

static void CallTransitionScript(sol::function& Script, HEntity EntityID, CStrID CurrState, CStrID NextState)
{
	if (Script.valid())
	{
		auto Result = Script(EntityID, CurrState, NextState);
		if (!Result.valid())
		{
			sol::error Error = Result;
			::Sys::Error(Error.what());
		}
	}
}
//---------------------------------------------------------------------

void ProcessStateChangeRequest(DEM::Game::CGameWorld& World, sol::state& Lua, HEntity EntityID,
	CSmartObjectComponent& SOComponent, CSmartObject& SOAsset)
{
	// In case game logic callbacks will make another request during the processing of this one
	const auto RequestedState = SOComponent.RequestedState;
	SOComponent.RequestedState = CStrID::Empty;

	const CSmartObjectTransitionInfo* pTransitionCN = nullptr;
	float InitialProgress = 0.f;

	// Initial state is always force-set
	if (!SOComponent.CurrState) SOComponent.Force = true;

	// Handle current transition (A->B) softly
	if (SOComponent.NextState && !SOComponent.Force)
	{
		// We are already transiting to the requested state: (A->B)->B
		if (RequestedState == SOComponent.NextState)
		{
			::Sys::Log("SmartObjects.ProcessStateChangeRequest() > already transiting to the requested state\n");
			return;
		}

		// Try to interrupt currently active transition: (A->B)->C or (A->B)->A
		// If current transition was deleted from asset in runtime, default to ResetToStart mode
		pTransitionCN = SOAsset.FindTransition(SOComponent.CurrState, SOComponent.NextState);
		const auto InterruptionMode = pTransitionCN ? pTransitionCN->InterruptionMode : ETransitionInterruptionMode::ResetToStart;
		switch (InterruptionMode)
		{
			case ETransitionInterruptionMode::ResetToStart:
			{
				// Cancel current transition
				break;
			}
			case ETransitionInterruptionMode::RewindToEnd:
			{
				// Finish current transition immediately
				// Arrive into the NextState, so CurrState = NextState, NextState = 0
				break;
			}
			case ETransitionInterruptionMode::Proportional:
			{
				const auto& Task = pTransitionCN->TimelineTask;
				const float FullTime = SOComponent.Player.GetLoopDuration() * Task.LoopCount;
				if (FullTime > 0.f) InitialProgress = SOComponent.Player.GetRemainingTime() / FullTime;

				// For (A->B)->A transition inverted progress has more meaning, because we go back the same way
				if (RequestedState == SOComponent.CurrState)
					InitialProgress = (1.f - InitialProgress);

				break;
			}
			case ETransitionInterruptionMode::Forbid:
			{
				// Current transition is kept, request is discarded
				//???don't reset request but wait for transition end? or must be in game logic?
				//???or special Wait mode, which will restore SOComponent.RequestedState and exit?
				::Sys::Log("SmartObjects.ProcessStateChangeRequest() > transition interruption is forbidden\n");
				return;
			}
			//???ETransitionInterruptionMode::Force?
		}
	}

	const CTimelineTask* pPrevTask = nullptr;
	Anim::PTimelineTrack CurrTrack;

	// Now current transition must be cancelled
	if (SOComponent.NextState)
	{
		if (pTransitionCN)
		{
			// Current TL track instance is reusable only if its prototype is known
			pPrevTask = &pTransitionCN->TimelineTask;
			CurrTrack = SOComponent.Player.GetTrack();
		}

		CallTransitionScript(SOAsset.GetScriptFunction(Lua, "OnTransitionCancel"), EntityID, SOComponent.CurrState, SOComponent.NextState);
		SOComponent.NextState = CStrID::Empty;
	}

	// Try to start a transition to the requested state
	if (!SOComponent.Force)
	{
		if (auto pTransitionCR = SOAsset.FindTransition(SOComponent.CurrState, RequestedState))
		{
			SOComponent.NextState = RequestedState;
			CallTransitionScript(SOAsset.GetScriptFunction(Lua, "OnTransitionStart"), EntityID, SOComponent.CurrState, SOComponent.NextState);
			RunTimelineTask(World, EntityID, SOComponent, pTransitionCR->TimelineTask, InitialProgress, CurrTrack, pPrevTask);
		}
		else
		{
			// No transition to the requested state found. Could discard a request, but let's force it instead.
			SOComponent.Force = true;
		}
	}

	// Force-set requested state
	if (SOComponent.Force && SOComponent.CurrState != RequestedState)
	{
		if (auto pState = SOAsset.FindState(RequestedState))
		{
			CallTransitionScript(SOAsset.GetScriptFunction(Lua, "OnStateForceSet"), EntityID, SOComponent.CurrState, RequestedState);
			RunTimelineTask(World, EntityID, SOComponent, pState->TimelineTask, 0.f, CurrTrack, pPrevTask);
			SOComponent.CurrState = RequestedState;
			SOComponent.UpdateScript = SOAsset.GetScriptFunction(Lua, "OnStateUpdate");
		}
	}
}
//---------------------------------------------------------------------

void UpdateSmartObjects(DEM::Game::CGameWorld& World, sol::state& Lua, float dt)
{
	//???only in certain level? or process all loaded object in the world, like now?
	World.ForEachEntityWith<CSmartObjectComponent>([&World, &Lua, dt](auto EntityID, auto& Entity, CSmartObjectComponent& SOComponent)
	{
		CSmartObject* pSmart = SOComponent.Asset->GetObject<CSmartObject>();
		if (!pSmart) return;

		// No need to make transition A->A (should not happen)
		n_assert_dbg(!SOComponent.NextState || SOComponent.CurrState != SOComponent.NextState);
		if (SOComponent.NextState && SOComponent.CurrState == SOComponent.NextState)
			SOComponent.NextState = CStrID::Empty;

		if (SOComponent.RequestedState) ProcessStateChangeRequest(World, Lua, EntityID, SOComponent, *pSmart);

		// Advance player time even if no timeline is associated with the state.
		// This time may be used in game logic and means how long we are in this state.
		// FIXME: prev VS curr time as the time since entering into the state!
		SOComponent.Player.Update(dt);

		if (SOComponent.NextState)
		{
			// Infinite transitions are not allowed, so zero loops always mean transition end
			if (!SOComponent.Player.GetRemainingLoopCount())
			{
				if (auto pState = pSmart->FindState(SOComponent.NextState))
				{
					const CTimelineTask* pPrevTask = nullptr;
					Anim::PTimelineTrack CurrTrack;
					if (auto pTransitionCN = pSmart->FindTransition(SOComponent.CurrState, SOComponent.NextState))
					{
						// Current TL track instance is reusable only if its prototype is known
						pPrevTask = &pTransitionCN->TimelineTask;
						CurrTrack = SOComponent.Player.GetTrack();
					}

					// End transition, enter the destination state
					CallTransitionScript(pSmart->GetScriptFunction(Lua, "OnTransitionEnd"), EntityID, SOComponent.CurrState, SOComponent.NextState);
					RunTimelineTask(World, EntityID, SOComponent, pState->TimelineTask, 0.f, CurrTrack, pPrevTask);
					SOComponent.CurrState = SOComponent.NextState;
					SOComponent.UpdateScript = pSmart->GetScriptFunction(Lua, "OnStateUpdate");
				}
				else
				{
					// State was deleted from asset in runtime, cancel to source state
					CallTransitionScript(pSmart->GetScriptFunction(Lua, "OnTransitionCancel"), EntityID, SOComponent.CurrState, SOComponent.NextState);
					SOComponent.Player.SetTrack(nullptr);
				}

				SOComponent.NextState = CStrID::Empty;
			}
		}
		else
		{
			// Update in the current state logic

			//???pSmart->OnStateUpdate(World, EntityID, SOComponent.CurrState)?

			//!!!TODO: call custom logic, if exists!

			if (SOComponent.UpdateScript.valid())
			{
				auto Result = SOComponent.UpdateScript(EntityID, SOComponent.CurrState);
				if (!Result.valid())
				{
					sol::error Error = Result;
					::Sys::Error(Error.what());
				}
			}
		}
	});
}
//---------------------------------------------------------------------

}
