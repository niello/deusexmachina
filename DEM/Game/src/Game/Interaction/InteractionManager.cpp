#include "InteractionManager.h"
#include <Game/Interaction/Interaction.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/Interaction/TargetFilter.h>
#include <Game/Objects/SmartObject.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/ECS/GameWorld.h>
#include <Data/Params.h>
#include <Data/DataArray.h>

namespace DEM::Game
{

//!!!must be per-session, because uses Lua, which is per-session!
//!!!if so, no need to store session inside an interaction context!
CInteractionManager::CInteractionManager(CGameSession& Owner)
	: _Lua(Owner.GetScriptState())
{
	//???right here or call methods from other CPPs? Like CTargetInfo::ScriptInterface(sol::state_view Lua)
	//!!!needs definition of pointer types!
	// TODO: add traits for HEntity, add bindings for vector3 and CSceneNode
	_Lua.new_usertype<CTargetInfo>("CTargetInfo"
		, "Entity", &CTargetInfo::Entity
		//, "Node", &CTargetInfo::pNode
		, "Point", &CTargetInfo::Point
		); // there is also &CTargetInfo::Valid

	_Lua.new_usertype<CInteractionContext>("CInteractionContext"
		, "Tool", &CInteractionContext::Tool
		, "Source", &CInteractionContext::Source
		, "Actors", &CInteractionContext::Actors
		, "CandidateTarget", &CInteractionContext::CandidateTarget
		, "Targets", &CInteractionContext::Targets
		, "SelectedTargetCount", &CInteractionContext::SelectedTargetCount
		);
}
//---------------------------------------------------------------------

CInteractionManager::~CInteractionManager() = default;
//---------------------------------------------------------------------

bool CInteractionManager::RegisterTool(CStrID ID, CInteractionTool&& Tool)
{
	auto It = _Tools.find(ID);
	if (It == _Tools.cend())
		_Tools.emplace(ID, std::move(Tool));
	else
		It->second = std::move(Tool);

	return true;
}
//---------------------------------------------------------------------

bool CInteractionManager::RegisterTool(CStrID ID, const Data::CParams& Params)
{
	CInteractionTool Tool;

	Tool.IconID = Params.Get(CStrID("Icon"), CString::Empty);
	Tool.Name = Params.Get(CStrID("Name"), CString::Empty);
	Tool.Description = Params.Get(CStrID("Description"), CString::Empty);
	const std::string IsAvailable = Params.Get(CStrID("IsAvailable"), CString::Empty);

	Data::PDataArray Tags;
	if (Params.TryGet<Data::PDataArray>(Tags, CStrID("Tags")))
		for (const auto& TagData : *Tags)
			Tool.Tags.insert(CStrID(TagData.GetValue<CString>().CStr()));

	Data::PDataArray Actions;
	if (Params.TryGet<Data::PDataArray>(Actions, CStrID("Actions")))
	{
		for (const auto& ActionData : *Actions)
		{
			if (auto pActionStr = ActionData.As<CString>())
			{
				Tool.Interactions.emplace_back(CStrID(pActionStr->CStr()), sol::function());
			}
			else if (auto pActionDesc = ActionData.As<Data::PParams>())
			{
				CString ActID;
				if ((*pActionDesc)->TryGet<CString>(ActID, CStrID("ID")))
				{
					// TODO: pushes the compiled chunk as a Lua function on top of the stack,
					// need to save anywhere in this Tool's table?
					sol::function ConditionFunc;
					const std::string Condition = (*pActionDesc)->Get(CStrID("Condition"), CString::Empty);
					if (!Condition.empty())
					{
						auto LoadedCondition = _Lua.load("local Actors, Target = ...; return " + Condition, (ID.CStr() + ActID).CStr());
						if (LoadedCondition.valid()) ConditionFunc = LoadedCondition;
					}

					Tool.Interactions.emplace_back(CStrID(ActID.CStr()), std::move(ConditionFunc));
				}
			}
		}
	}

	// TODO: pushes the compiled chunk as a Lua function on top of the stack,
	// need to save anywhere in this Tool's table?
	if (!IsAvailable.empty())
	{
		auto LoadedCondition = _Lua.load("local SelectedActors = ...; return " + IsAvailable, ID.CStr());
		if (LoadedCondition.valid()) Tool.AvailabilityCondition = LoadedCondition;
	}

	return RegisterTool(ID, std::move(Tool));
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

const CInteractionTool* CInteractionManager::FindTool(CStrID ID) const
{
	auto It = _Tools.find(ID);
	return (It == _Tools.cend()) ? nullptr : &It->second;
}
//---------------------------------------------------------------------

// Returns found tool only if it is available for the provided actor selection
const CInteractionTool* CInteractionManager::FindAvailableTool(CStrID ToolID, const std::vector<HEntity>& SelectedActors) const
{
	auto pTool = FindTool(ToolID);
	if (!pTool) return nullptr;

	if (!pTool->AvailabilityCondition) return pTool;

	auto Result = pTool->AvailabilityCondition(SelectedActors);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
	}

	return (Result.valid() && Result) ? pTool : nullptr;
}
//---------------------------------------------------------------------

const CInteraction* CInteractionManager::FindInteraction(CStrID ID) const
{
	auto It = _Interactions.find(ID);
	return (It == _Interactions.cend()) ? nullptr : It->second.get();
}
//---------------------------------------------------------------------

bool CInteractionManager::SelectTool(CInteractionContext& Context, CStrID ToolID, HEntity Source)
{
	if (Context.Tool == ToolID) return true;

	if (!FindAvailableTool(ToolID, Context.Actors)) return false;

	Context.Tool = ToolID;
	Context.Source = Source;
	ResetCandidateInteraction(Context);

	return false;
}
//---------------------------------------------------------------------

void CInteractionManager::ResetTool(CInteractionContext& Context)
{
	Context.Tool = CStrID::Empty;
	Context.Source = {};
	ResetCandidateInteraction(Context);
}
//---------------------------------------------------------------------

void CInteractionManager::ResetCandidateInteraction(CInteractionContext& Context)
{
	Context.Interaction = CStrID::Empty;
	Context.Condition = sol::function();
	Context.Targets.clear();
	Context.SelectedTargetCount = 0;
}
//---------------------------------------------------------------------

const CInteraction* CInteractionManager::ValidateInteraction(CStrID ID, const sol::function& Condition, CInteractionContext& Context)
{
	// Validate interaction itself
	auto pInteraction = FindInteraction(ID);
	if (!pInteraction) return nullptr;

	// Check additional condition
	if (Condition)
	{
		auto Result = Condition(Context.Actors, Context.CandidateTarget);
		if (!Result.valid())
		{
			sol::error Error = Result;
			::Sys::Error(Error.what());
			return nullptr;
		}
		else if (Result.get_type() == sol::type::nil || !Result) return nullptr; //???SOL: why nil can't be negated?
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
	auto pTool = FindAvailableTool(Context.Tool, Context.Actors);

	// If selected tool became unavailable, reset to default one
	if (!pTool)
	{
		SelectTool(Context, _DefaultTool, {});
		pTool = FindTool(Context.Tool);
		if (!pTool) return false;
	}

	// Validate current candidate interaction, if set
	if (Context.Interaction)
	{
		if (ValidateInteraction(Context.Interaction, Context.Condition, Context))
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

	n_assert(!Context.SelectedTargetCount);

	// If target is a smart object, select first applicable interaction in it
	if (Context.CandidateTarget.Entity)
	{
		//!!!remove session from context, store here as owner!
		if (auto pWorld = Context.Session->FindFeature<CGameWorld>())
		{
			auto pSmart = pWorld->FindComponent<CSmartObjectComponent>(Context.CandidateTarget.Entity);
			if (pSmart && pSmart->Asset)
			{
				if (auto pSmartAsset = pSmart->Asset->ValidateObject<CSmartObject>())
				{
					for (const CStrID ID : pSmartAsset->GetInteractions())
					{
						auto Condition = pSmartAsset->GetScriptFunction(Context.Session->GetScriptState(), "Can" + std::string(ID.CStr()));
						auto pInteraction = ValidateInteraction(ID, Condition, Context);
						if (pInteraction &&
							pInteraction->GetMaxTargetCount() > 0 &&
							pInteraction->GetTargetFilter(0)->IsTargetValid(Context))
						{
							Context.Interaction = ID;
							Context.Condition = Condition;
							return true;
						}
					}
				}
			}
		}
	}

	// Select first interaction in the current tool that accepts current target
	for (U32 i = 0; i < pTool->Interactions.size(); ++i)
	{
		const auto& [ID, Condition] = pTool->Interactions[i];
		auto pInteraction = ValidateInteraction(ID, Condition, Context);
		if (pInteraction &&
			pInteraction->GetMaxTargetCount() > 0 &&
			pInteraction->GetTargetFilter(0)->IsTargetValid(Context))
		{
			Context.Interaction = ID;
			Context.Condition = Condition;
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
	if (!Context.Interaction) return false;

	// FIXME: probably redundant search, may store InteractionID in a context if tool is not required here
	auto pTool = FindTool(Context.Tool);
	if (!pTool) return false;
	auto pInteraction = FindInteraction(Context.Interaction);
	if (!pInteraction) return false;

	if (Context.SelectedTargetCount >= pInteraction->GetMaxTargetCount()) return false;

	if (!pInteraction->GetTargetFilter(Context.SelectedTargetCount)->IsTargetValid(Context)) return false;

	// If just started to select targets, allocate slots for them
	if (Context.Targets.empty())
	{
		Context.Targets.resize(pInteraction->GetMaxTargetCount());
		Context.SelectedTargetCount = 0;
	}

	Context.Targets[Context.SelectedTargetCount] = Context.CandidateTarget;
	++Context.SelectedTargetCount;

	return true;
}
//---------------------------------------------------------------------

// Revert targets one by one, then revert non-default tool
bool CInteractionManager::Revert(CInteractionContext& Context)
{
	if (Context.SelectedTargetCount)
		--Context.SelectedTargetCount;
	else if (Context.Tool && Context.Tool != _DefaultTool)
		SelectTool(Context, _DefaultTool, {});
	else
		return false;
	return true;
}
//---------------------------------------------------------------------

bool CInteractionManager::ExecuteInteraction(CInteractionContext& Context, bool Enqueue)
{
	if (!Context.Interaction) return false;

	auto pTool = FindAvailableTool(Context.Tool, Context.Actors);
	if (!pTool) return false;
	auto pInteraction = FindInteraction(Context.Interaction);
	if (!pInteraction) return false;

	//!!!DBG TMP!
	std::string Actor = Context.Actors.empty() ? "none" : std::to_string(*Context.Actors.begin());
	::Sys::Log(("Tool: " + Context.Tool.ToString() +
		", Interaction: " + Context.Interaction.CStr() +
		", Actor: " + Actor + "\n").c_str());

	return pInteraction->Execute(Context, Enqueue);
}
//---------------------------------------------------------------------

const std::string& CInteractionManager::GetCursorImageID(CInteractionContext& Context) const
{
	//!!!DBG TMP!
	static const std::string EmptyString;

	auto pTool = FindTool(Context.Tool);
	if (!pTool) return EmptyString;
	auto pInteraction = FindInteraction(Context.Interaction);
	if (!pInteraction) return EmptyString;
	return pInteraction->GetCursorImageID(Context.SelectedTargetCount);
}
//---------------------------------------------------------------------

}
