#include "InteractionManager.h"
#include <Game/Interaction/Interaction.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/Objects/SmartObject.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/ECS/GameWorld.h>
#include <Data/Params.h>
#include <Data/DataArray.h>

namespace DEM::Game
{

CInteractionManager::CInteractionManager(CGameSession& Owner)
	: _Session(Owner)
{
	//???right here or call methods from other CPPs? Like CTargetInfo::ScriptInterface(sol::state_view Lua)
	//!!!needs definition of pointer types!
	// TODO: add traits for HEntity, add bindings for vector3 and CSceneNode
	_Session.GetScriptState().new_usertype<CTargetInfo>("CTargetInfo"
		, "Entity", &CTargetInfo::Entity
		//, "Node", &CTargetInfo::pNode
		, "Point", &CTargetInfo::Point
		); // there is also &CTargetInfo::Valid

	_Session.GetScriptState().new_usertype<CInteractionContext>("CInteractionContext"
		, "Tool", &CInteractionContext::Tool
		, "Source", &CInteractionContext::Source
		, "Actors", &CInteractionContext::Actors
		, "CandidateTarget", &CInteractionContext::CandidateTarget
		, "Targets", &CInteractionContext::Targets
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
						auto LoadedCondition = _Session.GetScriptState().load("local Actors, Target = ...; return " + Condition, (ID.CStr() + ActID).CStr());
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

bool CInteractionManager::RegisterInteraction(CStrID ID, PInteraction&& Interaction, CStrID SmartObjectID)
{
	if (!Interaction/* || !Interaction->GetMaxTargetCount()*/) return false;

	auto& Map = _Interactions[SmartObjectID];

	auto It = Map.find(ID);
	if (It == Map.cend())
		Map.emplace(ID, std::move(Interaction));
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
		if (pInteraction && pInteraction->IsAvailable(_Session, Context)) return pTool;
	}

	return nullptr;
}
//---------------------------------------------------------------------

const CInteraction* CInteractionManager::FindInteraction(CStrID ID, CStrID SmartObjectID) const
{
	// Search for an interaction with the specified smart object
	if (SmartObjectID)
	{
		auto ItMap = _Interactions.find(SmartObjectID);
		if (ItMap != _Interactions.cend())
		{
			auto It = (*ItMap).second.find(ID);
			if (It != (*ItMap).second.cend()) return It->second.get();
		}
	}

	// Search for a global interaction
	auto ItMap = _Interactions.find(CStrID::Empty);
	if (ItMap != _Interactions.cend())
	{
		auto It = (*ItMap).second.find(ID);
		if (It != (*ItMap).second.cend()) return It->second.get();
	}

	return nullptr;
}
//---------------------------------------------------------------------

bool CInteractionManager::SelectTool(CInteractionContext& Context, CStrID ToolID, HEntity Source) const
{
	if (Context.Tool == ToolID) return true;

	if (!FindAvailableTool(ToolID, Context)) return false;

	Context.Tool = ToolID;
	Context.Source = Source;
	ResetCandidateInteraction(Context);

	return false;
}
//---------------------------------------------------------------------

void CInteractionManager::ResetTool(CInteractionContext& Context) const
{
	Context.Tool = CStrID::Empty;
	Context.Source = {};
	ResetCandidateInteraction(Context);
}
//---------------------------------------------------------------------

void CInteractionManager::ResetCandidateInteraction(CInteractionContext& Context) const
{
	Context.Interaction = CStrID::Empty;
	Context.SmartObjectID = CStrID::Empty;
	Context.Targets.clear();
	Context.TargetExpected = ESoftBool::True;
}
//---------------------------------------------------------------------

bool CInteractionManager::UpdateCandidateInteraction(CInteractionContext& Context) const
{
	ZoneScoped;

	if (Context.Interaction)
	{
		// If we already started selecting targets, interaction remains selected if only it doesn't become invalid
		if (!Context.Targets.empty())
		{
			// Validate interaction itself
			if (auto pInteraction = FindInteraction(Context.Interaction, Context.SmartObjectID))
			{
				// Check main condition. Validate selected targets, their state might change.
				if (pInteraction->IsAvailable(_Session, Context) && pInteraction->AreSelectedTargetsValid(_Session, Context)) return true;
			}
		}

		ResetCandidateInteraction(Context);
	}

	//!!!TODO: single iact "tool" - use iact ID, don't reset to default tool. Need to check if iact with ID exists!

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
		if (auto pWorld = _Session.FindFeature<CGameWorld>())
		{
			auto pSmart = pWorld->FindComponent<CSmartObjectComponent>(Context.CandidateTarget.Entity);
			if (pSmart && pSmart->Asset) pSOAsset = pSmart->Asset->ValidateObject<CSmartObject>();
		}
	}

	// Resolve selected tool into an interaction, the first valid for the target becomes a candidate
	for (const auto& [OriginalID, Precondition] : pTool->Interactions)
	{
		// Check tool precondition
		//???!!!move to method?! or can simplify this call? too verbose without any complex logic
		if (Precondition)
		{
			auto Result = Precondition(Context.Actors, Context.CandidateTarget);
			if (!Result.valid())
			{
				sol::error Error = Result;
				::Sys::Error(Error.what());
				continue;
			}

			//???SOL: why nil can't be negated? https://www.lua.org/pil/3.3.html
			const auto Type = Result.get_type();
			if (Type == sol::type::none || Type == sol::type::nil || !Result) continue;
		}

		// Allow a smart object to override an interaction by ID
		// (e.g. Default -> OpenDoor / CloseDoor or SomeSpellAbility -> OpenDoorWhenSomeSpellCasted)
		const CFixedArray<CStrID>* pOverrides = pSOAsset ? pSOAsset->GetInteractionOverrides(OriginalID) : nullptr;
		if (pOverrides)
		{
			for (CStrID OverrideID : *pOverrides)
			{
				auto pInteraction = FindInteraction(OverrideID, pSOAsset->GetID());
				if (pInteraction &&
					pInteraction->IsAvailable(_Session, Context) &&
					pInteraction->IsCandidateTargetValid(_Session, Context))
				{
					Context.Interaction = OverrideID;
					Context.SmartObjectID = pSOAsset->GetID();
					return true;
				}
			}
		}
		else
		{
			// No overrides from a smart object, try original interaction
			auto pInteraction = FindInteraction(OriginalID);
			if (pInteraction &&
				pInteraction->IsAvailable(_Session, Context) &&
				pInteraction->IsCandidateTargetValid(_Session, Context))
			{
				Context.Interaction = OriginalID;
				Context.SmartObjectID = CStrID::Empty;
				return true;
			}
		}
	}

	return false;
}
//---------------------------------------------------------------------

bool CInteractionManager::AcceptTarget(CInteractionContext& Context) const
{
	if (!Context.Interaction || Context.TargetExpected == ESoftBool::False) return false;

	auto pInteraction = FindInteraction(Context.Interaction, Context.SmartObjectID);
	if (!pInteraction || !pInteraction->IsCandidateTargetValid(_Session, Context)) return false;

	Context.Targets.push_back(Context.CandidateTarget);
	Context.TargetExpected = pInteraction->NeedMoreTargets(Context);

	return true;
}
//---------------------------------------------------------------------

// Revert targets one by one, then revert non-default tool
bool CInteractionManager::Revert(CInteractionContext& Context) const
{
	if (!Context.Targets.empty())
	{
		auto pInteraction = FindInteraction(Context.Interaction, Context.SmartObjectID);
		if (!pInteraction) return false;

		Context.Targets.pop_back();
		Context.TargetExpected = pInteraction->NeedMoreTargets(Context);
	}
	else if (Context.Tool && Context.Tool != _DefaultTool)
		SelectTool(Context, _DefaultTool, {});
	else
		return false;
	return true;
}
//---------------------------------------------------------------------

bool CInteractionManager::ExecuteInteraction(CInteractionContext& Context, bool Enqueue) const
{
	// Ensure interaction and all mandatory targets are selected
	if (!Context.Interaction || Context.TargetExpected == ESoftBool::True) return false;

	auto pInteraction = FindInteraction(Context.Interaction, Context.SmartObjectID);
	if (!pInteraction) return false;

	//!!!DBG TMP!
	const std::string Actor = Context.Actors.empty() ? "none" : std::to_string(*Context.Actors.begin());
	const std::string SmartObjectID = Context.SmartObjectID ? Context.SmartObjectID.ToString() : "none";
	::Sys::Log(("Tool: " + Context.Tool.ToString() +
		", Interaction: " + Context.Interaction.ToString() +
		", SmartObject: " + SmartObjectID +
		", Actor: " + Actor + "\n").c_str());

	return pInteraction->Execute(_Session, Context, Enqueue);
}
//---------------------------------------------------------------------

const std::string& CInteractionManager::GetCursorImageID(CInteractionContext& Context) const
{
	//!!!DBG TMP!
	static const std::string EmptyString;

	auto pTool = FindTool(Context.Tool);
	if (!pTool) return EmptyString;
	auto pInteraction = FindInteraction(Context.Interaction, Context.SmartObjectID);
	if (!pInteraction) return EmptyString;
	return pInteraction->GetCursorImageID(Context.Targets.size());
}
//---------------------------------------------------------------------

}
