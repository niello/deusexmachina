#include <Game/ECS/GameWorld.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>

namespace DEM::Game
{

void ProcessStateChangeRequest(CSmartObjectComponent& SOComponent, const CSmartObject& SOAsset)
{
	// If already in another transition and not forced to the new state, need interruption
	if (!SOComponent.Force && SOComponent.NextState && SOComponent.RequestedState != SOComponent.NextState)
	{
		// Request currently active transition, it must exist
		if (auto pTransitionCN = SOAsset.FindTransition(SOComponent.CurrState, SOComponent.NextState))
		{
			switch (pTransitionCN->InterruptionMode)
			{
				case ETransitionInterruptionMode::ResetToStart:
				// - pTransitionCN is cancelled, new transition starts
					break;
				case ETransitionInterruptionMode::RewindToEnd:
				// - pTransitionCN is finished immediately, B becomes current state, new transition starts
					break;
				case ETransitionInterruptionMode::Proportional:
				// - pTransitionCN is cancelled, new transition starts, progress % is transferred to it
				// - For A->B->A case, progress % is inverted
					break;
				case ETransitionInterruptionMode::Forbid:
				{
					// Current transition is kept, request is discarded
					SOComponent.RequestedState = CStrID::Empty;
					::Sys::Log("SmartObjects.ProcessStateChangeRequest() > transition interruption is forbidden\n");
					return;
				}
			}

			//!!!if new transition not found, force!

			if (SOComponent.RequestedState == SOComponent.CurrState)
			{
				// (A->B)->A, try to switch to B->A with inverted progress (1-alpha)
				//???or for ABA keep setting too? Reset to A, Rewind to B, Proportional B->A inversion
			}
			else
			{
				// (A->B)->C, try to switch to A->C or B->C depending on interruption mode
			}
		}
		else
		{
			// Current transition was deleted in runtime, fallback to force-set
			SOComponent.Force = true;
		}
	}

	if (SOComponent.Force)
	{
		//force into requested state right now
		//do nothing if already in that state and not in transition
	}
	else
	{
		//init transition (must be found)
	}

	// Request is processed
	SOComponent.RequestedState = CStrID::Empty;
}
//---------------------------------------------------------------------

void UpdateSmartObjects(DEM::Game::CGameWorld& World, float dt)
{
	//???only in certain level? or process all loaded object in the world, like now?
	World.ForEachEntityWith<CSmartObjectComponent>([dt](auto EntityID, auto& Entity, CSmartObjectComponent& SOComponent)
	{
		CSmartObject* pSmart = SOComponent.Asset->GetObject<CSmartObject>();
		if (!pSmart) return;

		//!!!when init timeline, must check if timeline asset is the same for prev and curr cases!
		// if same, no actions required on it, only player must be updated.
		// if not same, must clone and bind to outputs

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
