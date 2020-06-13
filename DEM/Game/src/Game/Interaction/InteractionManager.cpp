#include "InteractionManager.h"
#include <Game/Interaction/Interaction.h>
#include <Game/Interaction/InteractionContext.h>
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

const IInteraction* CInteractionManager::FindInteraction(CStrID ID) const
{
	auto It = _Interactions.find(ID);
	return (It == _Interactions.cend()) ? nullptr : It->second.get();
}
//---------------------------------------------------------------------

bool CInteractionManager::SelectAbility(CInteractionContext& Context, CStrID AbilityID)
{
	if (Context.Ability == AbilityID) return true;

	auto pAbility = FindAvailableAbility(Context.Ability, Context.SelectedActors);
	if (!pAbility) return false;

	Context.Ability = AbilityID;
	ResetCandidateInteraction(Context);

	// immediately check ability actions for auto-satisfied targets, AcceptTarget if so
	//???only first interaction(s)? auto targets must not be mixed in one ability with selectable ones,
	//at least they must go first. If additional condition disables them, it is OK.
	//it is much like UpdateCandidateInteraction but with auto-confirmation of target

	return false;
}
//---------------------------------------------------------------------

void CInteractionManager::ResetCandidateInteraction(CInteractionContext& Context)
{
	Context.InteractionIndex = CInteractionContext::NO_INTERACTION;
	Context.SelectedTargets.clear();
	Context.SelectedTargetCount = 0;
}
//---------------------------------------------------------------------

const IInteraction* CInteractionManager::ValidateInteraction(const CAbility& Ability, U32 Index, CInteractionContext& Context)
{
	const auto& [ID, Condition] = Ability.Interactions[Index];

	// Check additional condition from the ability itself
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

	// Validate interaction itself
	auto pInteraction = FindInteraction(ID);
	if (!pInteraction) return nullptr;

	//???some Lua function for interaction availability check?

	// Validate selected targets, their state might change
	for (U32 i = 0; i < Context.SelectedTargetCount; ++i)
		if (!pInteraction->IsTargetValid(i, Context.SelectedTargets[i]))
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
		Context.Ability = _DefaultAbility;
		ResetCandidateInteraction(Context);

		pAbility = FindAbility(Context.Ability);
		if (!pAbility) return false;
	}

	// Validate current candidate interaction, if set
	if (Context.IsInteractionSet())
	{
		if (auto pInteraction = ValidateInteraction(*pAbility, Context.InteractionIndex, Context))
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

	// Select first interaction in the current ability that accepts current target
	for (U32 i = 0; i < pAbility->Interactions.size(); ++i)
	{
		auto pInteraction = ValidateInteraction(*pAbility, i, Context);
		if (pInteraction && pInteraction->IsTargetValid(0, Context.Target))
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
	if (Context.SelectedTargetCount >= Context.SelectedTargets.size()) return false;

	// if no target and next action target is not auto-satisfiable (Self, SourceItem etc), exit
	// validate current target against candidate action, exit if invalid

	if (Context.SelectedTargets.empty())
	{
		//Context.SelectedTargets.resize(Context.Interaction->GetMaxTargetCount());
		Context.SelectedTargetCount = 0;
	}

	// add target to the selected list
/*
	if (auto Target = _InteractionContext.Interaction->CreateTarget(_InteractionContext.SelectedTargetCount))
	{
		_InteractionContext.SelectedTargets[_InteractionContext.SelectedTargetCount] = Target;
		++_InteractionContext.SelectedTargetCount;
		if (_InteractionContext.SelectedTargetCount == _InteractionContext.SelectedTargets.size())
			_InteractionContext.Interaction->Execute();
	}
*/

	return false;
}
//---------------------------------------------------------------------

bool CInteractionManager::ExecuteInteraction(CInteractionContext& Context, bool Enqueue)
{
	return false;
}
//---------------------------------------------------------------------

}
