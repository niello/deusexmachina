#include "LogicRegistry.h"
#include <Game/GameSession.h>
#include <Scripting/ScriptCondition.h>

namespace DEM::Game
{

CLogicRegistry::CLogicRegistry(Game::CGameSession& Owner)
	: _Session(Owner)
{
	// Register standard condition implementations
	RegisterCondition<CFalseCondition>(CFalseCondition::Type);
	RegisterCondition<CAndCondition>(CAndCondition::Type);
	RegisterCondition<COrCondition>(COrCondition::Type);
	RegisterCondition<CNotCondition>(CNotCondition::Type);
	RegisterCondition<CVarCmpConstCondition>(CVarCmpConstCondition::Type);
	RegisterCondition<CVarCmpVarCondition>(CVarCmpVarCondition::Type);
	RegisterCondition<CLuaStringCondition>(CLuaStringCondition::Type);
}
//---------------------------------------------------------------------

CLogicRegistry::~CLogicRegistry() = default;
//---------------------------------------------------------------------

CScriptCondition* CLogicRegistry::RegisterScriptedCondition(CStrID Type, CStrID ScriptAssetID)
{
	if (auto ScriptObject = _Session.GetScript(ScriptAssetID))
	{
		auto It = _Conditions.insert_or_assign(Type, std::make_unique<CScriptCondition>(ScriptObject)).first;
		return static_cast<CScriptCondition*>(It->second.get());
	}

	return nullptr;
}
//---------------------------------------------------------------------

const CCommand& CLogicRegistry::RegisterScriptedCommand(CStrID Type, CStrID ScriptAssetID)
{
	if (auto ScriptObject = _Session.GetScript(ScriptAssetID))
	{
		if (auto FnExecute = ScriptObject.get<sol::function>("Execute"))
		{
			auto [It, Inserted] = _Commands.insert_or_assign(Type,
				[FnExecute](CGameSession& Session, const Data::CParams* pParams, CGameVarStorage* pVars)
			{
				FnExecute(pParams, pVars);
			});
			return It->second;
		}
	}

	return NopCommand;
}
//---------------------------------------------------------------------

}
