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

namespace Data
{
	class CParams;
}

namespace DEM::Game
{
class CGameSession;
using PCondition = std::unique_ptr<class ICondition>;
class CScriptCondition;
using CCommand = void (*)(CGameSession& Session, const Data::CParams* pParams, CGameVarStorage* pVars); //???std::function for binding Lua functions? Or ICommand?

class CLogicRegistry final : public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CLogicRegistry, DEM::Core::CRTTIBaseClass);

protected:

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

	void RegisterCommand(CStrID Type, CCommand Cmd)
	{
		_Commands.insert_or_assign(Type, Cmd);
	}

	CCommand FindCommand(CStrID Type) const
	{
		auto It = _Commands.find(Type);
		return (It != _Commands.cend()) ? It->second : nullptr;
	}
};

}
