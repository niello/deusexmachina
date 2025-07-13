#pragma once
#include "SolGame.h"
#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <Game/ECS/Components/EventsComponent.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Interaction/ScriptedAbility.h>
#include <Game/SessionVars.h>
#include <AI/CommandStackComponent.h>
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
template<> struct api_name<Game::HEntity> { static constexpr char* value = "Entity"; };
template<> struct api_name<rtm::vector4f> { static constexpr char* value = "Vector"; };

template<typename TVar, typename T>
static void RegisterVarStorageTemplateMethods(sol::usertype<T>& UserType)
{
	const std::string TypeAPIName = api_name_v<TVar> ? api_name_v<TVar> : typeid(TVar).name();

	UserType.set(std::string("Is") + TypeAPIName, &T::IsA<TVar>);

	// Only l-values can come from Lua to C++, can't use r-value references
	using TPass = std::conditional_t<DEM::Meta::should_pass_by_value<TVar>, TVar, const TVar&>;

	UserType.set(std::string("Set") + TypeAPIName, sol::overload(
		sol::resolve<HVar(CStrID, TPass)>(&T::Set<TPass>),
		[](T& Self, std::string_view ID, TPass Value) { return Self.Set<TPass>(CStrID(ID), Value); }));

	UserType.set(std::string("Get") + TypeAPIName, sol::overload(
		sol::resolve<TPass(HVar) const>(&T::Get<TVar>),
		sol::resolve<TPass(HVar, TPass) const>(&T::Get<TVar>),
		[](const T& Self, std::string_view ID) { return Self.Get<TVar>(Self.Find(CStrID(ID))); },
		[](const T& Self, std::string_view ID, TPass Dflt) { return Self.Get<TVar>(Self.Find(CStrID(ID)), Dflt); }));
}
//---------------------------------------------------------------------

template<typename... TVar>
static void RegisterVarStorageTemplateMethods(sol::usertype<CVarStorage<TVar...>>& UserType)
{
	(RegisterVarStorageTemplateMethods<TVar>(UserType), ...);
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
	RegisterVarStorage<Game::CGameVarStorage>(State, "CGameVarStorage");

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

	State.new_enum<DEM::AI::ECommandStatus>("EActionStatus",
		{
			{ "NotStarted", DEM::AI::ECommandStatus::NotStarted },
			{ "Running", DEM::AI::ECommandStatus::Running },
			{ "Succeeded", DEM::AI::ECommandStatus::Succeeded },
			{ "Failed", DEM::AI::ECommandStatus::Failed },
			{ "Cancelled", DEM::AI::ECommandStatus::Cancelled }
		}
	);

	State.new_usertype<DEM::Game::CAbilityInstance>("CAbilityInstance"
		, "Actor", &DEM::Game::CAbilityInstance::Actor
		, "Targets", &DEM::Game::CAbilityInstance::Targets
		, "Source", &DEM::Game::CAbilityInstance::Source
		, "ElapsedTime", &DEM::Game::CAbilityInstance::ElapsedTime
		, "PrevElapsedTime", &DEM::Game::CAbilityInstance::PrevElapsedTime
		, "Stop", [&World](DEM::Game::CAbilityInstance& AbilityInstance, DEM::AI::ECommandStatus Status)
		{
			auto pCmdStack = World.FindComponent<DEM::AI::CCommandStackComponent>(AbilityInstance.Actor);
			if (!pCmdStack) return;

			DEM::Game::HAction Action = pCmdStack->FindCurrent<DEM::Game::ExecuteAbility>();
			while (Action)
			{
				auto pAction = Action.As<DEM::Game::ExecuteAbility>();
				if (pAction && pAction->_AbilityInstance == &AbilityInstance)
				{
					pCmdStack->SetStatus(Action, Status);
					return;
				}

				Action = pCmdStack->FindCurrent<DEM::Game::ExecuteAbility>(Action);
			}
		}
	);

	State.new_usertype<DEM::Game::CScriptedAbilityInstance>("CScriptedAbilityInstance"
		, sol::base_classes, sol::bases<DEM::Game::CAbilityInstance>()
		, "Custom", &DEM::Game::CScriptedAbilityInstance::Custom
	);

	State.new_usertype<DEM::AI::CCommandStackComponent>("CCommandStackComponent"
		, "SetStatus", &DEM::AI::CCommandStackComponent::SetStatus
		, "GetCurrent", &DEM::AI::CCommandStackComponent::GetCurrent
		, "GetAbilityInstanceAction", [](DEM::AI::CCommandStackComponent& Self, const DEM::Game::CAbilityInstance& AbilityInstance)
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
