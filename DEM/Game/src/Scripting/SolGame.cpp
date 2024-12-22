#pragma once
#include "SolGame.h"
#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/EventsComponent.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Interaction/ScriptedAbility.h>
#include <Game/SessionVars.h>
#include <Animation/AnimationComponent.h>
#include <Scene/SceneComponent.h>
#include <Scripting/Flow/FlowAsset.h>

namespace DEM::Scripting
{
template<typename T> struct api_name { static constexpr char* value = nullptr; };
template<typename T> constexpr auto api_name_v = api_name<T>::value;
template<> struct api_name<bool> { static constexpr char* value = "Bool"; };
template<> struct api_name<int> { static constexpr char* value = "Int"; };
template<> struct api_name<float> { static constexpr char* value = "Float"; };
template<> struct api_name<std::string> { static constexpr char* value = "String"; };
template<> struct api_name<CStrID> { static constexpr char* value = "StrID"; };

template<typename U> using TPass = std::conditional_t<sizeof(U) <= sizeof(size_t), U, U&&>;

template<typename... TVar>
static void RegisterVarStorageTemplateMethods(sol::usertype<CVarStorage<TVar...>>& UserType)
{
	using T = CVarStorage<TVar...>;
	(UserType.set(std::string("Get") + (api_name_v<TVar> ? api_name_v<TVar> : typeid(T).name()), sol::overload(
		sol::resolve<typename T::TRetVal<TVar>(HVar) const>(&T::Get<TVar>),
		sol::resolve<typename T::TRetVal<TVar>(HVar, const TVar&) const>(&T::Get<TVar>),
		[](const T& Self, std::string_view ID) { return Self.Get<TVar>(Self.Find(CStrID(ID))); },
		[](const T& Self, std::string_view ID, TPass<TVar> Dflt) { return Self.Get<TVar>(Self.Find(CStrID(ID)), std::forward<TPass<TVar>>(Dflt)); }))
		, ...);
	(UserType.set(std::string("Is") + (api_name_v<TVar> ? api_name_v<TVar> : typeid(T).name()), &T::IsA<TVar>)
		, ...);
	(UserType.set(std::string("Set") + (api_name_v<TVar> ? api_name_v<TVar> : typeid(T).name()), sol::overload(
		sol::resolve<HVar(CStrID, TPass<TVar>)>(&T::Set<TVar>),
		[](T& Self, std::string_view ID, TPass<TVar> Value) { return Self.Set<TVar>(CStrID(ID), std::forward<TPass<TVar>>(Value)); }))
		, ...);
}
//---------------------------------------------------------------------

template<typename T>
sol::usertype<T> RegisterVarStorage(sol::state& State, std::string_view Key)
{
	// TODO: assert if the usertype for T already exists, or there will be an error later on (Lua will GC the second metatable)
	auto UserType = State.new_usertype<T>(Key
		, "new_shared", sol::factories([]() { return std::shared_ptr<T>(new T{}); }));
	UserType.set_function("clear", &T::clear);
	UserType.set_function("empty", &T::empty);
	UserType.set_function("size", &T::size);
	UserType.set_function("Find", static_cast<HVar(T::*)(CStrID ID) const>(&T::Find));
	UserType.set_function("Has", [](const T& Self, CStrID ID) { return !!Self.Find(ID); });
	RegisterVarStorageTemplateMethods(UserType);
	return UserType;
}
//---------------------------------------------------------------------

void RegisterGameTypes(sol::state& State, Game::CGameWorld& World)
{
	EnsureTable(State.globals(), { "DEM", "Flow" }).set_function("ResolveEntityID", &DEM::Flow::ResolveEntityID);

	RegisterVarStorage<CBasicVarStorage>(State, "CBasicVarStorage");

	State.new_usertype<Game::CSessionVars>("CSessionVars"
		, "Persistent", &Game::CSessionVars::Persistent
		, "Runtime", &Game::CSessionVars::Runtime
	);

	State.new_usertype<DEM::Game::CGameLevel>("CGameLevel"
		, "FindClosestEntity", sol::overload(
			&DEM::Game::CGameLevel::FindClosestEntity<std::nullptr_t>
			, &DEM::Game::CGameLevel::FindClosestEntity<sol::function>)
	);

	State.new_usertype<DEM::Game::CGameWorld>("CGameWorld"
		, sol::meta_function::index, [](DEM::Game::CGameWorld& Self, sol::stack_object Key) { return sol::object(Self._ScriptFields[Key]); }
		, "FindLevel", &DEM::Game::CGameWorld::FindLevel
		//, "FindEntity", [](DEM::Game::CGameWorld& Self, DEM::Game::HEntity EntityID) { return sol::nil;  }
	);

	auto UT_HEntity = State.new_usertype<DEM::Game::HEntity>("HEntity"
		, sol::constructors<sol::types<>, sol::types<const DEM::Game::HEntity&>>()
	);
	UT_HEntity.set("IsValid", [](DEM::Game::HEntity e) { return !!e; });
	UT_HEntity.set("IsEmpty", [](DEM::Game::HEntity e) { return !e; });
	UT_HEntity.set("Raw", &DEM::Game::HEntity::Raw);
	UT_HEntity.set("From", [](DEM::Game::HEntity::TRawValue Raw) { return DEM::Game::HEntity{ Raw }; });
	UT_HEntity.set("Empty", sol::var(DEM::Game::HEntity{}));
	RegisterStringOperations(UT_HEntity);

	State.new_usertype<DEM::Game::CSmartObjectComponent>("CSmartObjectComponent"
		, "CurrState", &DEM::Game::CSmartObjectComponent::CurrState
		, "NextState", &DEM::Game::CSmartObjectComponent::NextState
		, "RequestedState", &DEM::Game::CSmartObjectComponent::RequestedState
		, "OnTransitionStart", &DEM::Game::CSmartObjectComponent::OnTransitionStart
		, "OnTransitionCancel", &DEM::Game::CSmartObjectComponent::OnTransitionCancel
		, "OnStateChanged", &DEM::Game::CSmartObjectComponent::OnStateChanged
		, "RequestState", [](DEM::Game::CSmartObjectComponent& Self, CStrID State) { Self.RequestedState = State; Self.Force = false;  }
		, "ForceState", [](DEM::Game::CSmartObjectComponent& Self, CStrID State) { Self.RequestedState = State; Self.Force = true;  }
	);

	State.new_usertype<DEM::Game::CAnimationComponent>("CAnimationComponent"
		, "Controller", &DEM::Game::CAnimationComponent::Controller
	);

	State.new_usertype<DEM::Game::CSceneComponent>("CSceneComponent"
		, "RootNode", &DEM::Game::CSceneComponent::RootNode
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

	// For CEventsComponent
	DEM::Scripting::RegisterSignalType<void(DEM::Game::HEntity, CStrID, const Data::CParams*, float)>(State);

	State.new_usertype<DEM::Game::CEventsComponent>("CEventsComponent"
		, "OnEvent", &DEM::Game::CEventsComponent::OnEvent
	);
}
//---------------------------------------------------------------------

}
