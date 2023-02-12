#pragma once
#include "SolGame.h"
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Interaction/ScriptedAbility.h>
#include <Animation/AnimationComponent.h>

namespace DEM::Scripting
{

void RegisterGameTypes(sol::state& State, Game::CGameWorld& World)
{
	State.new_usertype<DEM::Game::CGameWorld>("CGameWorld"
		, sol::meta_function::index, [](DEM::Game::CGameWorld& Self, sol::stack_object Key) { return sol::object(Self._ScriptFields[Key]); }
		//, "FindEntity", [](DEM::Game::CGameWorld& Self, DEM::Game::HEntity EntityID) { return sol::nil;  }
	);

	State.new_usertype<DEM::Game::HEntity>("HEntity"
		, sol::constructors<sol::types<>, sol::types<const DEM::Game::HEntity&>>()
		, sol::meta_function::to_string, &DEM::Game::EntityToString
		, sol::meta_function::concatenation, sol::overload(
			[](const char* a, DEM::Game::HEntity b) { return a + DEM::Game::EntityToString(b); }
			, [](DEM::Game::HEntity a, const char* b) { return DEM::Game::EntityToString(a) + b; })
		, "IsValid", [](DEM::Game::HEntity e) { return !!e; }
		, "IsEmpty", [](DEM::Game::HEntity e) { return !e; }
	);
	sol::table HEntityTable = State["HEntity"];
	HEntityTable.set("Empty", DEM::Game::HEntity{});

	State.new_usertype<DEM::Game::CSmartObjectComponent>("CSmartObjectComponent"
		, "CurrState", &DEM::Game::CSmartObjectComponent::CurrState
		, "NextState", &DEM::Game::CSmartObjectComponent::NextState
		, "RequestedState", &DEM::Game::CSmartObjectComponent::RequestedState
		, "OnTransitionStart", &DEM::Game::CSmartObjectComponent::OnTransitionStart
		, "OnTransitionEnd", &DEM::Game::CSmartObjectComponent::OnTransitionEnd
		, "OnTransitionCancel", &DEM::Game::CSmartObjectComponent::OnTransitionCancel
		, "OnStateForceSet", &DEM::Game::CSmartObjectComponent::OnStateForceSet
		, "RequestState", [](DEM::Game::CSmartObjectComponent& Self, CStrID State) { Self.RequestedState = State; Self.Force = false;  }
		, "ForceState", [](DEM::Game::CSmartObjectComponent& Self, CStrID State) { Self.RequestedState = State; Self.Force = true;  }
	);

	State.new_usertype<DEM::Game::CAnimationComponent>("CAnimationComponent"
		, "Controller", &DEM::Game::CAnimationComponent::Controller
	);

	State.new_enum<DEM::Game::EFacingMode>("EFacingMode",
		{
			{ "None", DEM::Game::EFacingMode::None },
			{ "Direction", DEM::Game::EFacingMode::Direction },
			{ "Point", DEM::Game::EFacingMode::Point },
		}
	);

	State.new_usertype<DEM::Game::CFacingParams>("CFacingParams"
		, "Dir", &DEM::Game::CFacingParams::Dir
		, "Tolerance", &DEM::Game::CFacingParams::Tolerance
		, "Mode", &DEM::Game::CFacingParams::Mode
	);

	State.new_enum<DEM::Game::EActionStatus>("EActionStatus",
		{
			{ "Active", DEM::Game::EActionStatus::Active },
			{ "Succeeded", DEM::Game::EActionStatus::Succeeded },
			{ "Failed", DEM::Game::EActionStatus::Failed },
			{ "Cancelled", DEM::Game::EActionStatus::Cancelled },
			{ "NotQueued", DEM::Game::EActionStatus::NotQueued }
		}
	);

	State.new_usertype<DEM::Game::CAbilityInstance>("CAbilityInstance"
		, "Actor", &DEM::Game::CAbilityInstance::Actor
		, "Targets", &DEM::Game::CAbilityInstance::Targets
		, "Source", &DEM::Game::CAbilityInstance::Source
		, "ElapsedTime", &DEM::Game::CAbilityInstance::ElapsedTime
		, "PrevElapsedTime", &DEM::Game::CAbilityInstance::PrevElapsedTime
		, "Stop", [&World](DEM::Game::CAbilityInstance& AbilityInstance, DEM::Game::EActionStatus Status)
		{
			auto pQueue = World.FindComponent<DEM::Game::CActionQueueComponent>(AbilityInstance.Actor);
			if (!pQueue) return;

			DEM::Game::HAction Action = pQueue->FindCurrent<DEM::Game::ExecuteAbility>();
			while (Action)
			{
				auto pAction = Action.As<DEM::Game::ExecuteAbility>();
				if (pAction && pAction->_AbilityInstance == &AbilityInstance)
				{
					pQueue->SetStatus(Action, Status);
					return;
				}

				Action = pQueue->FindCurrent<DEM::Game::ExecuteAbility>(Action);
			}
		}
	);

	State.new_usertype<DEM::Game::CScriptedAbilityInstance>("CScriptedAbilityInstance"
		, sol::base_classes, sol::bases<DEM::Game::CAbilityInstance>()
		, "Custom", &DEM::Game::CScriptedAbilityInstance::Custom
	);

	State.new_usertype<DEM::Game::CActionQueueComponent>("CActionQueueComponent"
		, "SetStatus", &DEM::Game::CActionQueueComponent::SetStatus
		, "GetCurrent", &DEM::Game::CActionQueueComponent::GetCurrent
		, "GetRoot", &DEM::Game::CActionQueueComponent::GetRoot
		, "GetAbilityInstanceAction", [](DEM::Game::CActionQueueComponent& Self, const DEM::Game::CAbilityInstance& AbilityInstance)
		{
			DEM::Game::HAction Action = Self.FindCurrent<DEM::Game::ExecuteAbility>();
			while (Action)
			{
				auto pAction = Action.As<DEM::Game::ExecuteAbility>();
				if (pAction && pAction->_AbilityInstance == &AbilityInstance) return Action;

				Action = Self.FindCurrent<DEM::Game::ExecuteAbility>(Action);
			}
			return DEM::Game::HAction{};
		}
	);
}
//---------------------------------------------------------------------

}
