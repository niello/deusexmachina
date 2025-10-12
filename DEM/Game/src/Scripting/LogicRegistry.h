#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/StringID.h>

// A registry of declarative condition implementations for picking them by type ID

//???CConditionData -> CLogicData / CLogicDesc / CLogicCallData / ...? identical for conditions and actions.
//???can optimize static logic params from PParams to CGameVarStorage or something like that?
//!!!source and target can be written to context vars, like in Flow! See ResolveEntityID, same as for e.g. conversation Initiator.

namespace DEM::Game
{
class CGameSession;
using PCondition = std::unique_ptr<class ICondition>;
class CScriptCondition;

class CLogicRegistry final : public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CLogicRegistry, DEM::Core::CRTTIBaseClass);

protected:

	CGameSession&                          _Session;
	std::unordered_map<CStrID, PCondition> _Conditions;

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
};

}
