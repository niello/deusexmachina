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
	Context.Targets.clear();
	Context.SelectedTargetCount = 0;
}
//---------------------------------------------------------------------

const CInteraction* CInteractionManager::ValidateInteraction(CStrID ID, const CInteractionContext& Context) const
{
	// Validate interaction itself
	auto pInteraction = FindInteraction(ID);
	if (!pInteraction) return nullptr;

	// Check main condition
	if (!pInteraction->IsAvailable(Context)) return nullptr;

	// Validate selected targets, their state might change
	if (!pInteraction->AreSelectedTargetsValid(_Session, Context)) return nullptr;

	return pInteraction;
}
//---------------------------------------------------------------------

bool CInteractionManager::UpdateCandidateInteraction(CInteractionContext& Context)
{
	if (Context.Interaction)
	{
		// If we already started selecting targets, interaction remains selected if only it doesn't become invalid
		if (Context.SelectedTargetCount && ValidateInteraction(Context.Interaction, Context)) return true;

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
	for (const auto& [OriginalID, Condition] : pTool->Interactions)
	{
		// Check tool precondition
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
			else if (/*Result.get_type() == sol::type::nil ||*/ !Result) continue; //???SOL: why nil can't be negated? https://www.lua.org/pil/3.3.html
		}

		CStrID ID = OriginalID;

		// Allow a smart object to override an interaction by ID
		// (e.g. Default -> OpenDoor or FireballAbility -> MySpecialReactionOnFireball)
		//!!!remember SO ID only if overridden interaction is SO-specific (can override to another global iact!)
		//!!!???can have the same ID but different logic? Name clash between global & SO (not SO & SO)?
		if (pSOAsset)
			if (const CStrID OverrideID = pSOAsset->GetInteractionOverride(ID, Context))
				ID = OverrideID;

		auto pInteraction = FindInteraction(ID);
		if (pInteraction &&
			pInteraction->IsAvailable(Context) &&
			pInteraction->IsCandidateTargetValid(_Session, Context))
		{
			Context.Interaction = ID;
			return true;
		}
	}

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

	if (!pInteraction->IsCandidateTargetValid(_Session, Context)) return false;

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
		--Context.SelectedTargetCount; // NB: doesn't clear target info, so it must not have any strong refs!
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

	return pInteraction->Execute(_Session, Context, Enqueue);
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
