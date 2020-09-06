#include "InteractionManager.h"
#include <Game/Interaction/Interaction.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/Interaction/TargetFilter.h>
#include <Game/ECS/GameWorld.h>
#include <Data/Params.h>
#include <Data/DataArray.h>

namespace DEM::Game
{

CInteractionManager::CInteractionManager(sol::state_view Lua)
	: _Lua(std::move(Lua))
{
	//???right here or call methods from other CPPs? Like CTargetInfo::ScriptInterface(sol::state_view Lua)
	//!!!needs definition of pointer types!
	// TODO: add traits for HEntity, add bindings for vector3 and CSceneNode
	_Lua.new_usertype<CTargetInfo>("TargetInfo",
		"Entity", &CTargetInfo::Entity,
		//"Node", &CTargetInfo::pNode,
		"Point", &CTargetInfo::Point); // there is also &CTargetInfo::Valid
}
//---------------------------------------------------------------------

CInteractionManager::~CInteractionManager() = default;
//---------------------------------------------------------------------

bool CInteractionManager::RegisterAbility(CStrID ID, CAbility&& Ability)
{
	auto It = _Abilities.find(ID);
	if (It == _Abilities.cend())
		_Abilities.emplace(ID, std::move(Ability));
	else
		It->second = std::move(Ability);

	return true;
}
//---------------------------------------------------------------------

bool CInteractionManager::RegisterAbility(CStrID ID, const Data::CParams& Params)
{
	CAbility Ability;

	Ability.IconID = Params.Get(CStrID("Icon"), CString::Empty);
	Ability.Name = Params.Get(CStrID("Name"), CString::Empty);
	Ability.Description = Params.Get(CStrID("Description"), CString::Empty);
	const std::string IsAvailable = Params.Get(CStrID("IsAvailable"), CString::Empty);

	Data::PDataArray Tags;
	if (Params.TryGet<Data::PDataArray>(Tags, CStrID("Tags")))
		for (const auto& TagData : *Tags)
			Ability.Tags.insert(CStrID(TagData.GetValue<CString>().CStr()));

	Data::PDataArray Actions;
	if (Params.TryGet<Data::PDataArray>(Actions, CStrID("Actions")))
	{
		for (const auto& ActionData : *Actions)
		{
			if (auto pActionStr = ActionData.As<CString>())
			{
				Ability.Interactions.emplace_back(CStrID(pActionStr->CStr()), sol::function());
			}
			else if (auto pActionDesc = ActionData.As<Data::PParams>())
			{
				CString ActID;
				if ((*pActionDesc)->TryGet<CString>(ActID, CStrID("ID")))
				{
					// TODO: pushes the compiled chunk as a Lua function on top of the stack,
					// need to save anywhere in this Ability's table?
					sol::function ConditionFunc;
					const std::string Condition = (*pActionDesc)->Get(CStrID("Condition"), CString::Empty);
					if (!Condition.empty())
					{
						auto LoadedCondition = _Lua.load("local Target = ...; return " + Condition, (ID.CStr() + ActID).CStr());
						if (LoadedCondition.valid()) ConditionFunc = LoadedCondition;
					}

					Ability.Interactions.emplace_back(CStrID(ActID.CStr()), std::move(ConditionFunc));
				}
			}
		}
	}

	// TODO: pushes the compiled chunk as a Lua function on top of the stack,
	// need to save anywhere in this Ability's table?
	if (!IsAvailable.empty())
	{
		auto LoadedCondition = _Lua.load("local SelectedActors = ...; return " + IsAvailable, ID.CStr());
		if (LoadedCondition.valid()) Ability.AvailabilityCondition = LoadedCondition;
	}

	return RegisterAbility(ID, std::move(Ability));
}
//---------------------------------------------------------------------

bool CInteractionManager::RegisterInteraction(CStrID ID, PInteraction&& Interaction)
{
	auto It = _Interactions.find(ID);
	if (It == _Interactions.cend())
		_Interactions.emplace(ID, std::move(Interaction));
	else
		It->second = std::move(Interaction);

	return true;
}
//---------------------------------------------------------------------

const CAbility* CInteractionManager::FindAbility(CStrID ID) const
{
	auto It = _Abilities.find(ID);
	return (It == _Abilities.cend()) ? nullptr : &It->second;
}
//---------------------------------------------------------------------

// Returns found ability only if it is available for the provided actor selection
const CAbility* CInteractionManager::FindAvailableAbility(CStrID AbilityID, const std::vector<HEntity>& SelectedActors) const
{
	auto pAbility = FindAbility(AbilityID);
	if (!pAbility) return nullptr;

	if (!pAbility->AvailabilityCondition) return pAbility;

	auto Result = pAbility->AvailabilityCondition(SelectedActors);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
	}

	return (Result.valid() && Result) ? pAbility : nullptr;
}
//---------------------------------------------------------------------

const CInteraction* CInteractionManager::FindInteraction(CStrID ID) const
{
	auto It = _Interactions.find(ID);
	return (It == _Interactions.cend()) ? nullptr : It->second.get();
}
//---------------------------------------------------------------------

bool CInteractionManager::SelectAbility(CInteractionContext& Context, CStrID AbilityID, HEntity AbilitySource)
{
	if (Context.Ability == AbilityID) return true;

	if (!FindAvailableAbility(AbilityID, Context.SelectedActors)) return false;

	Context.Ability = AbilityID;
	Context.AbilitySource = AbilitySource;
	ResetCandidateInteraction(Context);

	return false;
}
//---------------------------------------------------------------------

void CInteractionManager::ResetAbility(CInteractionContext& Context)
{
	Context.Ability = CStrID::Empty;
	Context.AbilitySource = {};
	ResetCandidateInteraction(Context);
}
//---------------------------------------------------------------------

void CInteractionManager::ResetCandidateInteraction(CInteractionContext& Context)
{
	Context.InteractionIndex = CInteractionContext::NO_INTERACTION;
	Context.SelectedTargets.clear();
	Context.SelectedTargetCount = 0;
}
//---------------------------------------------------------------------

const CInteraction* CInteractionManager::ValidateInteraction(CStrID ID, sol::function Condition, CInteractionContext& Context)
{
	// Validate interaction itself
	auto pInteraction = FindInteraction(ID);
	if (!pInteraction) return nullptr;

	// Check additional condition
	if (Condition)
	{
		auto Result = Condition(Context.Target);
		if (!Result.valid())
		{
			sol::error Error = Result;
			::Sys::Error(Error.what());
			return nullptr;
		}
		else if (!Result) return nullptr;
	}

	// Validate selected targets, their state might change
	for (U32 i = 0; i < Context.SelectedTargetCount; ++i)
		if (!pInteraction->GetTargetFilter(i)->IsTargetValid(Context, i))
			return nullptr;

	return pInteraction;
}
//---------------------------------------------------------------------

bool CInteractionManager::UpdateCandidateInteraction(CInteractionContext& Context)
{
	auto pAbility = FindAvailableAbility(Context.Ability, Context.SelectedActors);

	// If selected ability became unavailable, reset to default one
	if (!pAbility)
	{
		SelectAbility(Context, _DefaultAbility, {});
		pAbility = FindAbility(Context.Ability);
		if (!pAbility) return false;
	}

	// Validate current candidate interaction, if set
	if (Context.IsInteractionSet())
	{
		const auto& [ID, Condition] = pAbility->Interactions[Context.InteractionIndex];
		if (ValidateInteraction(ID, Condition, Context))
		{
			// If we already started to select targets, stick to the selected interaction
			if (Context.SelectedTargetCount) return true;
		}
		else
		{
			ResetCandidateInteraction(Context);
			return false;
		}
	}

	// If target is a smart object, select first appilcable interaction in it
	if (Context.Target.Entity)
	{
		if (auto pWorld = Context.Session->FindFeature<CGameWorld>())
		{
			//if (auto pSmart = pWorld->FindComponent<DEM::AI::CSmartObjectComponent>(Context.Target.Entity))
			{
				//get a list of interactions there
				//process the same way as pAbility->Interactions below
			}
		}
	}

	// Select first interaction in the current ability that accepts current target
	for (U32 i = 0; i < pAbility->Interactions.size(); ++i)
	{
		const auto& [ID, Condition] = pAbility->Interactions[i];
		auto pInteraction = ValidateInteraction(ID, Condition, Context);
		if (pInteraction &&
			pInteraction->GetMaxTargetCount() > 0 &&
			pInteraction->GetTargetFilter(0)->IsTargetValid(Context))
		{
			Context.InteractionIndex = i;
			return true;
		}
	}

	// No interaction found for the current context state
	ResetCandidateInteraction(Context);
	return false;
}
//---------------------------------------------------------------------

bool CInteractionManager::AcceptTarget(CInteractionContext& Context)
{
	if (!Context.IsInteractionSet()) return false;

	// FIXME: probably redundant search, may store InteractionID in a context if ability is not required here
	auto pAbility = FindAbility(Context.Ability);
	if (!pAbility) return false;
	auto pInteraction = FindInteraction(pAbility->Interactions[Context.InteractionIndex].first);
	if (!pInteraction) return false;

	if (Context.SelectedTargetCount >= pInteraction->GetMaxTargetCount()) return false;

	if (!pInteraction->GetTargetFilter(Context.SelectedTargetCount)->IsTargetValid(Context)) return false;

	// If just starget to select targets, allocate slots for them
	if (Context.SelectedTargets.empty())
	{
		Context.SelectedTargets.resize(pInteraction->GetMaxTargetCount());
		Context.SelectedTargetCount = 0;
	}

	Context.SelectedTargets[Context.SelectedTargetCount] = Context.Target;
	++Context.SelectedTargetCount;

	return true;
}
//---------------------------------------------------------------------

bool CInteractionManager::ExecuteInteraction(CInteractionContext& Context, bool Enqueue)
{
	if (!Context.IsInteractionSet()) return false;

	auto pAbility = FindAvailableAbility(Context.Ability, Context.SelectedActors);
	if (!pAbility) return false;
	const CStrID InteractionID = pAbility->Interactions[Context.InteractionIndex].first;
	auto pInteraction = FindInteraction(InteractionID);
	if (!pInteraction) return false;

	//!!!DBG TMP!
	std::string Actor = Context.SelectedActors.empty() ? "none" : std::to_string(*Context.SelectedActors.begin());
	::Sys::Log(("Ability: " + Context.Ability.ToString() +
		", Interaction: " + InteractionID.CStr() +
		", Actor: " + Actor + "\n").c_str());

	return pInteraction->Execute(Context, Enqueue);
}
//---------------------------------------------------------------------

const std::string& CInteractionManager::GetCursorImageID(CInteractionContext& Context) const
{
	//!!!DBG TMP!
	static const std::string EmptyString;

	auto pAbility = FindAbility(Context.Ability);
	if (!pAbility) return EmptyString;
	auto pInteraction = FindInteraction(pAbility->Interactions[Context.InteractionIndex].first);
	if (!pInteraction) return EmptyString;
	return pInteraction->GetCursorImageID(Context.SelectedTargetCount);
}
//---------------------------------------------------------------------

}
