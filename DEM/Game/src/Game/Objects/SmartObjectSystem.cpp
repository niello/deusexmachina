#include <Game/ECS/GameWorld.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>

namespace DEM::Game
{

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

		switch (SOComponent.Status)
		{
			case ESmartObjectStatus::InState:
			{
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

				//???!!!even if no state TL, may want to count time in this state (time from enter)!
				SOComponent.Player.Update(dt);

				break;
			}
			case ESmartObjectStatus::InTransition:
			{
				//if transition must end
				//   set TL to state TL or clear
				//   call OnStateExit
				//   call OnStateEnter
				//   clear next state
				// else
				//   update transition TL with dt
				break;
			}
			case ESmartObjectStatus::TransitionRequested:
			{
				//!!!need to know if in transition and in what transition!
				// TargetState & RequestedState instead of NextState?
				//???state? State, TransitionRequest, ForceRequest, Transition

				// if transition must start ([transition and??] TL are required, or it will be force-set)
				//   set TL to transition TL
				//   call OnStateStartExiting
				//   call OnStateStartEntering
				break;
			}
			case ESmartObjectStatus::ForceStateRequested:
			{
				//!!!need to know if in transition and in what transition!
				// TargetState & RequestedState instead of NextState?
				//   set TL to state TL or clear
				//   call OnStateExit
				//   call OnStateEnter
				//   clear next state
				break;
			}
		};
	});
}
//---------------------------------------------------------------------

}
