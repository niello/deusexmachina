#pragma once
#include <Scripting/Flow/FlowCommon.h> // for CFlowVarStorage only, maybe redeclare as CCommonVarStorage
#include <Game/ECS/Entity.h> // For ResolveEntityID utility method
#include <Data/Params.h>
#include <Data/Metadata.h>

// Condition object that can be described declaratively and evaluated in a provided context

namespace DEM::Game
{
	using PGameSession = Ptr<class CGameSession>;
}

namespace DEM::Events
{
	class CConnection;
}

namespace DEM::Flow
{
using PCondition = std::unique_ptr<class ICondition>;

struct CConditionData
{
	CStrID        Type;   // Empty type means no condition, always evaluates to 'true'
	Data::PParams Params;
};

struct CConditionContext
{
	const CConditionData&  Condition;
	Game::CGameSession&    Session;
	const CFlowVarStorage& Vars;
};

bool EvaluateCondition(const CConditionData& Cond, Game::CGameSession& Session, const CFlowVarStorage& Vars);
std::string GetConditionText(const CConditionData& Cond, Game::CGameSession& Session, const CFlowVarStorage& Vars);
Game::HEntity ResolveEntityID(const Data::PParams& Params, CStrID ParamID, const CFlowVarStorage& Vars);

class ICondition
{
public:

	virtual ~ICondition() = default;

	virtual bool Evaluate(const CConditionContext& Ctx) const = 0;
	virtual void GetText(std::string& Out, const CConditionContext& Ctx) const {}
	virtual void SubscribeRelevantEvents(std::vector<Events::CConnection>& OutSubs, const CConditionContext& Ctx, const std::function<void()>& Callback) const {}
};

class CFalseCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("False");

	virtual bool Evaluate(const CConditionContext& Ctx) const override { return false; }
	virtual void GetText(std::string& Out, const CConditionContext& Ctx) const { Out.append("[NEVER]"); }
};

class CAndCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("And");

	virtual bool Evaluate(const CConditionContext& Ctx) const override;
};

class COrCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("Or");

	virtual bool Evaluate(const CConditionContext& Ctx) const override;
};

class CNotCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("Not");

	virtual bool Evaluate(const CConditionContext& Ctx) const override;
};

class CVarCmpConstCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("VarCmpConst");

	virtual bool Evaluate(const CConditionContext& Ctx) const override;
	virtual void GetText(std::string& Out, const CConditionContext& Ctx) const;
};

class CVarCmpVarCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("VarCmpVar");

	virtual bool Evaluate(const CConditionContext& Ctx) const override;
};

class CLuaStringCondition : public ICondition
{
public:

	inline static const auto Type = CStrID("LuaString");

	virtual bool Evaluate(const CConditionContext& Ctx) const override;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::Flow::CConditionData>() { return "DEM::Flow::CConditionData"; }
template<> constexpr auto RegisterMembers<DEM::Flow::CConditionData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Flow::CConditionData, Type),
		DEM_META_MEMBER_FIELD(Flow::CConditionData, Params)
	);
}

}
