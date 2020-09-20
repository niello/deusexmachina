#include <Game/ECS/GameWorld.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>

namespace DEM::Game
{

void ProcessStateChangeRequest(CSmartObjectComponent& SOComponent, const CSmartObject& SOAsset)
{
	// In case game logic callbacks will make another request during the processing of this one
	const auto RequestedState = SOComponent.RequestedState;
	SOComponent.RequestedState = CStrID::Empty;

	const CSmartObjectTransitionInfo* pTransitionCN = nullptr;
	float InitialProgress = 0.f;

	// Handle current transition (A->B)
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
				// Remember progress % of the current transition
				// Cancel current transition
				// For A->B->C case, progress % is transferred as is
				// For A->B->A case, progress % is inverted (1-a)
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

	if (SOComponent.NextState)
	{
		//!!!if has current transition, must cancel it here!
		//!!!NB: we may reuse timeline, so real destruction must not happen here!
	}

	if (!SOComponent.Force)
	{
		if (auto pTransitionCR = SOAsset.FindTransition(SOComponent.CurrState, RequestedState))
		{
			SOComponent.NextState = RequestedState;
			if (pTransitionCN && pTransitionCR->TimelineTask.Timeline != pTransitionCN->TimelineTask.Timeline)
			{
				// clone new timeline instance for the player
				// bind to outputs
			}
			// set initial progress
			// initialize player with task
		}
		else
		{
			// No transition to the requested state found. Could discard a request, but let's force it instead.
			SOComponent.Force = true;
		}
	}

	if (SOComponent.Force)
	{
		//force into requested state right now, so CurrState = RequestedState
		//do nothing if already in that state and not in transition
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

		if (SOComponent.NextState)
		{
			//transition
			// no logic update, timeline does that
			// check if ended
		}
		else
		{
			// Update in the current state

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

			// Advance player time even if no timeline is associated with the state.
			// This time may be used in game logic and means how long we are in this state.
			// FIXME: prev VS curr time!
			SOComponent.Player.Update(dt);
		}
	});
}
//---------------------------------------------------------------------

}
