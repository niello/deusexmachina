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

	// FIXME: tags not here, in actor abilities?!
	//Data::PDataArray Tags;
	//if (Params.TryGet<Data::PDataArray>(Tags, CStrID("Tags")))
	//	for (const auto& TagData : *Tags)
	//		Tool.Tags.insert(CStrID(TagData.GetValue<CString>().CStr()));

	Data::PDataArray Actions;
	if (Params.TryGet<Data::PDataArray>(Actions, CStrID("Interactions")))
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
						else
						{
							sol::error Error = LoadedCondition;
							::Sys::Error(Error.what());
						}
					}

					Tool.Interactions.emplace_back(CStrID(ActID.CStr()), std::move(ConditionFunc));
				}
			}
		}
	}

	return RegisterTool(ID, std::move(Tool));
}
//---------------------------------------------------------------------

bool CInteractionManager::RegisterInteraction(CStrID ID, PInteraction&& Interaction)
{
	if (!Interaction || !Interaction->GetMaxTargetCount()) return false;

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

// Returns found tool only if it is available in a context
const CInteractionTool* CInteractionManager::FindAvailableTool(CStrID ID, const CInteractionContext& Context) const
{
	auto pTool = FindTool(ID);
	if (!pTool) return nullptr;

	//???!!!check Condition too?!
	for (const auto& [InteractionID, Condition] : pTool->Interactions)
	{
		auto pInteraction = FindInteraction(InteractionID);
		if (pInteraction && pInteraction->IsAvailable(Context)) return pTool;
	}

	return nullptr;
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

	if (!FindAvailableTool(ToolID, Context)) return false;

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

const CInteraction* CInteractionManager::ValidateInteraction(CStrID ID, const sol::function& Condition, CInteractionContext& Context) const
{
	// Validate interaction itself
	auto pInteraction = FindInteraction(ID);
	if (!pInteraction) return nullptr;

	// Check main condition
	if (!pInteraction->IsAvailable(Context)) return nullptr;

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
	if (Context.Interaction)
	{
		// If we already started selecting targets, interaction remains selected if only it doesn't become invalid
		if (Context.SelectedTargetCount && ValidateInteraction(Context.Interaction, Context.Condition, Context)) return true;

		ResetCandidateInteraction(Context);
	}

	// Find current tool by ID. If not found, reset the tool to default.
	auto pTool = FindTool(Context.Tool);
	if (!pTool)
	{
		if (Context.Tool != _DefaultTool)
		{
			Context.Tool = _DefaultTool;
			Context.Source = {};
			pTool = FindTool(Context.Tool);
		}

		if (!pTool) return false;
	}

	// Check if our current target candidate is a smart object
	CSmartObject* pSOAsset = nullptr;
	if (Context.CandidateTarget.Entity)
	{
		//!!!remove session from context, store here as owner! Anyway CInteractionManager is per session.
		if (auto pWorld = Context.Session->FindFeature<CGameWorld>())
		{
			auto pSmart = pWorld->FindComponent<CSmartObjectComponent>(Context.CandidateTarget.Entity);
			if (pSmart && pSmart->Asset) pSOAsset = pSmart->Asset->ValidateObject<CSmartObject>();
		}
	}

	// Allow a smart object to override an interaction by tool ID (e.g. DefaultAction -> Open/Close for door)
	if (pSOAsset) // && Context.Tool)
	{
		//Interaction+ID (or empty) = pSOAsset->GetInteractionOverride(Context.Tool, Context);
		// if has override, return with it!
		//!!!must check availability of SO interaction in the context!

		/* OLD:
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
		*/
	}

	// Resolve selected tool into an interaction, the first valid for the target becomes a candidate
	//bool HasAvailableInteraction = false;
	for (const auto& [ID, Condition] : pTool->Interactions)
	{
		auto pInteraction = FindInteraction(ID);
		if (!pInteraction || !pInteraction->IsAvailable(Context)) continue;

		//HasAvailableInteraction = true;

		// Check additional condition
		//???!!!move to method?! or can simplify this call? too verbose without any complex logic
		// FIXME: DUPLICATED CODE! See ValidateInteraction!
		if (Condition)
		{
			auto Result = Condition(Context.Actors, Context.CandidateTarget);
			if (!Result.valid())
			{
				sol::error Error = Result;
				::Sys::Error(Error.what());
				continue;
			}
			else if (Result.get_type() == sol::type::nil || !Result) continue; //???SOL: why nil can't be negated? https://www.lua.org/pil/3.3.html
		}

		// If interaction accepts our candidate target as its first target, it becomes our choice
		if (pInteraction->GetTargetFilter(0)->IsTargetValid(Context))
		{
			Context.Interaction = ID;
			Context.Condition = Condition; //???precondition from tool only? availability is virtualized in an interaction
			break;
		}
	}

	// Allow a smart object to override an interaction by ID (e.g. FireballAbility -> MySpecialReactionOnFireball)
	if (pSOAsset && Context.Interaction)
	{
		//Interaction+ID (or empty) = pSOAsset->GetInteractionOverride(Context.Interaction, Context);
		// if has override, return with it!
		//!!!must check availability of SO interaction in the context!
	}

	// If tool has no available interactions, the tool itself is not available and must be reset to default
	//???realy need here? allow an application to do this if it wants?
	//if (!HasAvailableInteraction)
	//{
	//	Context.Tool = _DefaultTool;
	//	Context.Source = {};
	//}

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

	auto pTool = FindAvailableTool(Context.Tool, Context);
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
