#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/GameVarStorage.h>
#include <Data/StringID.h>

// A registry of declarative condition implementations for picking them by type ID

//???CConditionData -> CLogicData / CLogicDesc / CLogicCallData / ...? identical for conditions and actions.
//???can optimize static logic params from PParams to CGameVarStorage or something like that?
//!!!source and target can be written to context vars, like in Flow! See ResolveEntityID, same as for e.g. conversation Initiator.
//???CConditionContext -> CLogicContext / CLogicCallParams? likely identical for conditions and actions! Data (type + static params), Session, optional Vars (dynamic ctx)
//conditions accept const vars, so maube must separate CConditionContext and CCommandContext, but the main difference will be pVars constness.
//Condition (including chance) is external to the low level command desc? Or optional addition to any command?! Parse like in CConditionData but as an additional opt. field?

//???command list as a command?
//???behaviour/script/commandlist is a vector<command + optional condition [+ optional chance]>?

//???how about resolving conditions and commands from type right on deserialization? session-aware serialization?
//!!!command list can be deserialized from file and then resolved (both commands and conditions) with a second session-aware pass!!!
//but if command list is a resource, then can't pre-init it. May store as part of resource without resolving. Or register logic per app, not per session?
//can add Flow actions that executes a command list and a single command, to easily reuse registered logic pieces in Flow scripts

//!!!need script interface! access conditions(?) and commands(!) to call them from script directly, e.g. Session.Logic.PlayVFX(...)

namespace Data
{
	class CParams;
}

namespace DEM::Game
{
class CGameSession;
using PCondition = std::unique_ptr<class ICondition>;
class CScriptCondition;
using CCommand = std::function<void(CGameSession& Session, const Data::CParams* pParams, CGameVarStorage* pVars)>;

class CLogicRegistry final : public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CLogicRegistry, DEM::Core::CRTTIBaseClass);

protected:

	static inline CCommand NopCommand;

	CGameSession&                          _Session;
	std::unordered_map<CStrID, PCondition> _Conditions;
	std::unordered_map<CStrID, CCommand>   _Commands;

public:

	CLogicRegistry(CGameSession& Owner);
	~CLogicRegistry();

	template<typename T, typename... TArgs>
	T* RegisterCondition(CStrID Type, TArgs&&... Args)
	{
		auto It = _Conditions.insert_or_assign(Type, std::make_unique<T>(std::forward<TArgs>(Args)...)).first;
		return static_cast<T*>(It->second.get());
	}

	CScriptCondition* RegisterScriptedCondition(CStrID Type, CStrID ScriptAssetID);

	ICondition* FindCondition(CStrID Type) const
	{
		auto It = _Conditions.find(Type);
		return (It != _Conditions.cend()) ? It->second.get() : nullptr;
	}

	void RegisterCommand(CStrID Type, const CCommand& Cmd)
	{
		_Commands.insert_or_assign(Type, Cmd);
	}

	void RegisterCommand(CStrID Type, CCommand&& Cmd)
	{
		_Commands.insert_or_assign(Type, std::move(Cmd));
	}

	const CCommand& FindCommand(CStrID Type) const
	{
		auto It = _Commands.find(Type);
		return (It != _Commands.cend()) ? It->second : NopCommand;
	}
};

}
