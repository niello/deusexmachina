#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/GameVarStorage.h>
#include <Data/StringID.h>

// A registry of declarative condition implementations for picking them by type ID

//???could deserialize static data of commands and conditions into C++ structures and keep them instead of CParams?
//???how about resolving conditions and commands from type right on deserialization? session-aware serialization?
//!!!command list can be deserialized from file without session and then resolved (both commands and conditions) with a second session-aware pass!!!
//but if command list is a resource, then can't pre-init it. May store as part of resource without resolving. Or register logic per app, not per session?

// TODO FLOW: can add Flow actions that execute a command list and a single command, to easily reuse registered logic pieces in Flow scripts

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
using CCommand = std::function<bool(CGameSession& Session, const Data::CParams* pParams, CGameVarStorage* pVars)>;

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

	const CCommand& RegisterScriptedCommand(CStrID Type, CStrID ScriptAssetID);

	const CCommand& FindCommand(CStrID Type) const
	{
		auto It = _Commands.find(Type);
		return (It != _Commands.cend()) ? It->second : NopCommand;
	}
};

}
