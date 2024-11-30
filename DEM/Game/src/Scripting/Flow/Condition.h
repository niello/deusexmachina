#pragma once
#include <Scripting/Flow/FlowCommon.h> // for CFlowVarStorage only, maybe redeclare as CCommonVarStorage
#include <Data/Params.h>
#include <Data/Metadata.h>

// Condition object that can be described declaratively and evaluated in a provided context

namespace DEM::Game
{
	using PGameSession = Ptr<class CGameSession>;
}

namespace DEM::Flow
{
using PCondition = std::unique_ptr<class ICondition>;

struct CConditionData
{
	CStrID        Type;   // Empty type means no condition, always evaluates to 'true'
	Data::PParams Params;
};

bool EvaluateCondition(const CConditionData& Cond, Game::CGameSession& Session, const CFlowVarStorage& Vars);
std::string GetConditionText(const CConditionData& Cond, Game::CGameSession& Session, const CFlowVarStorage& Vars);

class ICondition
{
public:

	virtual ~ICondition() = default;

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const = 0;
	virtual void GetText(std::string& Out, const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const {}

	//???register signal objects of different session features under global names? or access right here from CGameSession arg?
	virtual void RENAME_SOMETHING_ABOUT_SUBSCRIPTIONS() const { /* return set of signals / event names or make subscriptions to dispatcher right here */ }
};

class CFalseCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("False");

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override { return false; }
	virtual void GetText(std::string& Out, const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const { Out.append("[NEVER]"); }
};

class CAndCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("And");

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override;
};

class COrCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("Or");

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override;
};

class CNotCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("Not");

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override;
};

class CVarCmpConstCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("VarCmpConst");

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override;
	virtual void GetText(std::string& Out, const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const;
};

class CVarCmpVarCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("VarCmpVar");

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override;
};

class CLuaStringCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("LuaString");

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::Flow::CConditionData>() { return "DEM::Flow::CConditionData"; }
template<> inline constexpr auto RegisterMembers<DEM::Flow::CConditionData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Flow::CConditionData, 1, Type),
		DEM_META_MEMBER_FIELD(Flow::CConditionData, 2, Params)
	);
}

}
