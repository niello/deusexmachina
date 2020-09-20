#include <Game/ECS/GameWorld.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>

namespace DEM::Game
{

static void RunTimelineTask(CSmartObjectComponent& SOComponent, const CTimelineTask& Task, float InitialProgress,
	Anim::PTimelineTrack&& CurrTrack, const Anim::PTimelineTrack& TrackProto)
{
	if (!Task.Timeline)
	{
		SOComponent.Player.SetTrack(nullptr);
		return;
	}

	const auto* pTaskTrackProto = Task.Timeline->GetObject<Anim::CTimelineTrack>();
	if (!CurrTrack || pTaskTrackProto != TrackProto)
	{
		CurrTrack = pTaskTrackProto->Clone();
		//!!!TODO:
		//if pose track:
		//CurrTrack->SetOutput(pAnimComponent->Output);
	}

	SOComponent.Player.SetTrack(CurrTrack);

	//???always relative time? may need absolute, especially when editing single multisegment TL.
	const float TrackDuration = SOComponent.Player.GetTrack()->GetDuration();
	SOComponent.Player.SetStartTime(Task.StartTime * TrackDuration);
	SOComponent.Player.SetEndTime(Task.EndTime * TrackDuration);
	SOComponent.Player.SetSpeed(Task.Speed);

	// Transfer the progress % from the previous transition, if required
	const float FullTime = SOComponent.Player.GetLoopDuration() * Task.LoopCount;
	SOComponent.Player.SetTime(InitialProgress * FullTime);

	SOComponent.Player.Play(Task.LoopCount);
}
//---------------------------------------------------------------------

void ProcessStateChangeRequest(CSmartObjectComponent& SOComponent, const CSmartObject& SOAsset)
{
	// In case game logic callbacks will make another request during the processing of this one
	const auto RequestedState = SOComponent.RequestedState;
	SOComponent.RequestedState = CStrID::Empty;

	const CSmartObjectTransitionInfo* pTransitionCN = nullptr;
	float InitialProgress = 0.f;

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

	Anim::PTimelineTrack TrackProto;
	Anim::PTimelineTrack CurrTrack;

	// Now current transition must be cancelled
	if (SOComponent.NextState)
	{
		if (pTransitionCN)
		{
			// Current TL track instance is reusable only if its prototype is known
			TrackProto = pTransitionCN->TimelineTask.Timeline->GetObject<Anim::CTimelineTrack>();
			CurrTrack = SOComponent.Player.GetTrack();
		}

		// call OnTransitionCancel

		SOComponent.NextState = CStrID::Empty;
	}

	// Try to start a transition to the requested state
	if (!SOComponent.Force)
	{
		if (auto pTransitionCR = SOAsset.FindTransition(SOComponent.CurrState, RequestedState))
		{
			SOComponent.NextState = RequestedState;

			// call OnTransitionStart

			RunTimelineTask(SOComponent, pTransitionCR->TimelineTask, InitialProgress, std::move(CurrTrack), TrackProto);
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
			SOComponent.CurrState = RequestedState;

			// call OnStateExit, OnStateEnter etc

			RunTimelineTask(SOComponent, pState->TimelineTask, 0.f, std::move(CurrTrack), TrackProto);
		}
	}
}
//---------------------------------------------------------------------

void UpdateSmartObjects(DEM::Game::CGameWorld& World, float dt)
{
	//???only in certain level? or process all loaded object in the world, like now?
	World.ForEachEntityWith<CSmartObjectComponent>([dt](auto EntityID, auto& Entity, CSmartObjectComponent& SOComponent)
	{
		CSmartObject* pSmart = SOComponent.Asset->GetObject<CSmartObject>();
		if (!pSmart) return;

		// No need to make transition A->A (should not happen)
		n_assert_dbg(SOComponent.CurrState != SOComponent.NextState);
		if (SOComponent.CurrState == SOComponent.NextState)
			SOComponent.NextState = CStrID::Empty;

		if (SOComponent.RequestedState) ProcessStateChangeRequest(SOComponent, *pSmart);

		// Advance player time even if no timeline is associated with the state.
		// This time may be used in game logic and means how long we are in this state.
		// FIXME: prev VS curr time as the time since entering into the state!
		SOComponent.Player.Update(dt);

		if (SOComponent.NextState)
		{
			// Infinite transitions are not allowed, so zero loops always mean transition end
			if (!SOComponent.Player.GetRemainingLoopCount())
			{
				Anim::PTimelineTrack TrackProto;
				Anim::PTimelineTrack CurrTrack;
				if (auto pTransitionCN = pSmart->FindTransition(SOComponent.CurrState, SOComponent.NextState))
				{
					// Current TL track instance is reusable only if its prototype is known
					TrackProto = pTransitionCN->TimelineTask.Timeline->GetObject<Anim::CTimelineTrack>();
					CurrTrack = SOComponent.Player.GetTrack();
				}

				if (auto pState = pSmart->FindState(SOComponent.NextState))
				{
					SOComponent.CurrState = SOComponent.NextState;

					//!!!call OnTransitionEnd!

					// call OnStateExit, OnStateEnter etc

					RunTimelineTask(SOComponent, pState->TimelineTask, 0.f, std::move(CurrTrack), TrackProto);
				}
				else
				{
					// If the state was deleted from asset in runtime, cancel to source state

					//!!!call OnTransitionCancel!

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

			auto& Script = pSmart->GetOnStateUpdateScript();
			if (Script.valid())
			{
				auto Result = Script(EntityID, SOComponent.CurrState);
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
