#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>
#include <Scripting/SolGame.h>

namespace DEM::Game
{

static void RunTimelineTask(CGameWorld& World, HEntity EntityID, Anim::CTimelinePlayer& Player,
	const Anim::CTimelineTask& Task, const Anim::CTimelineTask* pPrevTask, float InitialProgress = 0.f)
{
	n_assert_dbg(InitialProgress >= 0.f && InitialProgress <= 1.f);

	if (!Anim::LoadTimelineTaskIntoPlayer(Player, World, EntityID, Task, pPrevTask)) return;

	// FIXME: if set full progress, will timeline controller decrement loops?! SetTime or Rewind? or handle externally (here)?
	n_assert(InitialProgress < 1.f);

	// FIXME: more than one loop will now work wrong with initial progress!
	n_assert(Task.LoopCount < 2);

	// Transfer the progress % from the previous transition if required
	const float FullDuration = Player.GetLoopDuration() * Task.LoopCount;
	Player.SetTime(InitialProgress * std::copysign(FullDuration, Task.Speed));

	Player.Play(Task.LoopCount);
}
//---------------------------------------------------------------------

static void CallStateChangeScript(sol::function& Script, HEntity EntityID, CStrID CurrState, CStrID NextState)
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

static void ChangeState(CGameWorld& World, CGameSession& Session, HEntity EntityID, CSmartObjectComponent& Component, CStrID PrevState, const CSmartObjectStateInfo& State, const Anim::CTimelineTask* pPrevTask, bool ResetVisuals)
{
	Component.CurrState = State.ID;

	auto pSmart = Component.Asset->ValidateObject<CSmartObject>();
	CallStateChangeScript(pSmart->GetScriptFunction(Session, "OnStateChanged"), EntityID, PrevState, State.ID);
	Component.OnStateChanged(EntityID, PrevState, State.ID);

	if (State.TimelineTask.Timeline)
	{
		RunTimelineTask(World, EntityID, Component.Player, State.TimelineTask, pPrevTask);
	}
	else if (ResetVisuals)
	{
		// Try to set visual state from any transition connected to this state
		const Anim::CTimelineTask* pTask = nullptr;
		float InitialProgress = 0.f;

		// First try the first frame of any transition from this state
		for (auto& TransitionInfo : State.Transitions)
		{
			if (TransitionInfo.TimelineTask.Timeline)
			{
				pTask = &TransitionInfo.TimelineTask;
				break;
			}
		}

		// Then try the last frame of any transition to this state
		if (!pTask)
		{
			for (const auto& PrevState : pSmart->GetStates())
			{
				if (&PrevState == &State) continue;

				auto pTransition = pSmart->FindTransition(PrevState.ID, State.ID);
				if (pTransition && pTransition->TimelineTask.Timeline)
				{
					pTask = &pTransition->TimelineTask;
					InitialProgress = 0.999f; // FIXME: 1.f asserts at the time of writing this, need to improve timeline code!
					break;
				}
			}
		}

		//!!!FIXME: need better way to force set state of static objects!
		//!!!this is bad to simply play first frame of the whole timeline task! there can be undesired events, sounds etc.
		//!!!need to just evaluate animation poses!?
		if (pTask)
		{
			RunTimelineTask(World, EntityID, Component.Player, *pTask, pPrevTask, InitialProgress);

			//!!!FIXME: reversed timeline at 0 time doesn't sample the last frame!
			Component.Player.Update((InitialProgress > 0.f) ? 0.f : 0.0001f);

			Component.Player.SetTrack(nullptr);
		}
	}
}
//---------------------------------------------------------------------

void ProcessStateChangeRequest(CGameWorld& World, CGameSession& Session, HEntity EntityID,
	CSmartObjectComponent& SOComponent, const CSmartObject& SOAsset)
{
	// In case game logic callbacks will make another request during the processing of this one
	const auto RequestedState = SOComponent.RequestedState;
	SOComponent.RequestedState = CStrID::Empty;

	auto pRequestedState = SOAsset.FindState(RequestedState);
	if (!pRequestedState)
	{
		::Sys::Error("Missing smart object state requested!"); // TODO: normal logs and/or assertion!
		return;
	}

	const CSmartObjectTransitionInfo* pTransitionCN = nullptr;
	float InitialProgress = 0.f;

	// Initial state is always force-set
	if (!SOComponent.CurrState) SOComponent.Force = true;

	CStrID FromState = SOComponent.CurrState;

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
				NOT_IMPLEMENTED;
				break;
			}
			case ETransitionInterruptionMode::RewindToEnd:
			{
				// Finish current transition immediately
				// Arrive into the NextState, so CurrState = NextState, NextState = 0
				NOT_IMPLEMENTED;
				FromState = SOComponent.NextState; //!!!not needed if arrival happens here, it will handle C = N!
				break;
			}
			case ETransitionInterruptionMode::Proportional:
			{
				const auto& Task = pTransitionCN->TimelineTask;
				const float FullTime = SOComponent.Player.GetLoopDuration() * Task.LoopCount;
				if (FullTime > 0.f) InitialProgress = 1.f - (SOComponent.Player.GetRemainingTime() / FullTime);

				// For (A->B)->A transition inverted progress has more meaning, because we go back the same way
				if (RequestedState == SOComponent.CurrState)
				{
					InitialProgress = (1.f - InitialProgress);
					FromState = SOComponent.NextState;
				}

				break;
			}
			case ETransitionInterruptionMode::Forbid:
			{
				// Current transition is kept, request is discarded
				::Sys::Log("SmartObjects.ProcessStateChangeRequest() > transition interruption is forbidden\n");
				return;
			}
			case ETransitionInterruptionMode::Force:
			{
				SOComponent.Force = true;
				break;
			}
			case ETransitionInterruptionMode::Wait:
			{
				SOComponent.RequestedState = RequestedState;
				return;
			}
		}
	}

	const Anim::CTimelineTask* pPrevTask = nullptr;

	// Now current transition must be cancelled
	if (SOComponent.NextState)
	{
		// Current TL track instance is reusable only if its prototype is known
		if (pTransitionCN) pPrevTask = &pTransitionCN->TimelineTask;

		CallStateChangeScript(SOAsset.GetScriptFunction(Session, "OnTransitionCancel"), EntityID, SOComponent.CurrState, SOComponent.NextState);
		SOComponent.OnTransitionCancel(EntityID, SOComponent.CurrState, SOComponent.NextState);
		SOComponent.NextState = CStrID::Empty;
	}

	// Try to start a transition to the requested state
	if (!SOComponent.Force)
	{
		if (auto pTransitionCR = SOAsset.FindTransition(FromState, RequestedState))
		{
			SOComponent.CurrState = FromState;
			SOComponent.NextState = RequestedState;
			CallStateChangeScript(SOAsset.GetScriptFunction(Session, "OnTransitionStart"), EntityID, SOComponent.CurrState, SOComponent.NextState);
			SOComponent.OnTransitionStart(EntityID, SOComponent.CurrState, SOComponent.NextState);
			RunTimelineTask(World, EntityID, SOComponent.Player, pTransitionCR->TimelineTask, pPrevTask, InitialProgress);
		}
		else
		{
			// No transition to the requested state found. Could discard a request, but let's force it instead.
			SOComponent.Force = true;
		}
	}

	// Force-set requested state
	if (SOComponent.Force && FromState != RequestedState)
		ChangeState(World, Session, EntityID, SOComponent, FromState, *pRequestedState, pPrevTask, true);
}
//---------------------------------------------------------------------

void UpdateSmartObjects(CGameWorld& World, CGameSession& Session, float dt)
{
	World.ForEachEntityWith<CSmartObjectComponent>([&World, &Session, dt](auto EntityID, auto& Entity, CSmartObjectComponent& SOComponent)
	{
		if (!SOComponent.Asset) return;

		const CSmartObject* pSOAsset = SOComponent.Asset->GetObject<CSmartObject>();
		if (!pSOAsset) return;

		// No need to make transition A->A (should not happen)
		n_assert_dbg(!SOComponent.NextState || SOComponent.CurrState != SOComponent.NextState);
		if (SOComponent.NextState && SOComponent.CurrState == SOComponent.NextState)
			SOComponent.NextState = CStrID::Empty;

		if (SOComponent.RequestedState)
			ProcessStateChangeRequest(World, Session, EntityID, SOComponent, *pSOAsset);

		// Advance player time even if no timeline is associated with the state.
		// This time may be used in game logic and means how long we are in this state.
		// FIXME: prev VS curr time as the "time since entering into the state"?
		SOComponent.Player.Update(dt);

		if (SOComponent.NextState)
		{
			// Infinite transitions are not allowed, so zero loops always mean transition end
			if (!SOComponent.Player.GetRemainingLoopCount())
			{
				if (auto pState = pSOAsset->FindState(SOComponent.NextState))
				{
					// Current TL track instance is reusable only if its prototype is known
					const Anim::CTimelineTask* pPrevTask = nullptr;
					if (auto pTransitionCN = pSOAsset->FindTransition(SOComponent.CurrState, SOComponent.NextState))
						pPrevTask = &pTransitionCN->TimelineTask;

					// End transition, enter the destination state
					ChangeState(World, Session, EntityID, SOComponent, SOComponent.CurrState, *pState, pPrevTask, false);
				}
				else
				{
					// State was deleted from asset at runtime, cancel to source state
					CallStateChangeScript(pSOAsset->GetScriptFunction(Session, "OnTransitionCancel"), EntityID, SOComponent.CurrState, SOComponent.NextState);
					SOComponent.OnTransitionCancel(EntityID, SOComponent.CurrState, SOComponent.NextState);
					SOComponent.Player.SetTrack(nullptr);
				}

				SOComponent.NextState = CStrID::Empty;
			}
		}
		else
		{
			// Update the current state logic
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

// FIXME: how to init SO created in runtime?
void InitSmartObjects(CGameWorld& World, CGameSession& Session, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<CSmartObjectComponent>([&World, &Session, &ResMgr](auto EntityID, CSmartObjectComponent& SOComponent)
	{
		ResMgr.RegisterResource<CSmartObject>(SOComponent.Asset);
		if (!SOComponent.Asset) return;

		if (auto pSmart = SOComponent.Asset->ValidateObject<CSmartObject>())
		{
			pSmart->InitInSession(Session);

			SOComponent.UpdateScript = pSmart->GetScriptFunction(Session, "OnStateUpdate");

			// Force set initial state. Can be distunguished by empty previous state ID.
			const CStrID InitialState = SOComponent.CurrState ? SOComponent.CurrState : pSmart->GetDefaultState();
			if (auto pState = pSmart->FindState(InitialState))
				ChangeState(World, Session, EntityID, SOComponent, CStrID::Empty, *pState, nullptr, true);
		}
	});
}
//---------------------------------------------------------------------

}
