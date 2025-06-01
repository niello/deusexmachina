#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/StringID.h>

// A registry of declarative condition implementations for picking them by type ID

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::Flow
{
using PCondition = std::unique_ptr<class ICondition>;
class CScriptCondition;

class CConditionRegistry final : public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CConditionRegistry, DEM::Core::CRTTIBaseClass);

protected:

	Game::CGameSession&                    _Session;
	std::unordered_map<CStrID, PCondition> _Conditions;

public:

	CConditionRegistry(Game::CGameSession& Owner);
	~CConditionRegistry();

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
