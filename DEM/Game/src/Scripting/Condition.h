#pragma once
#include <Game/GameVarStorage.h>
#include <Data/Params.h>
#include <Data/Metadata.h>

// Condition object that can be described declaratively and evaluated in a provided context

namespace DEM::Events
{
	class CConnection;
}

namespace DEM::Game
{
using PCondition = std::unique_ptr<class ICondition>;
class CGameSession;

struct CConditionData
{
	CStrID        Type;   // Empty type means no condition, always evaluates to 'true'
	Data::PParams Params;
};

struct CConditionContext
{
	const CConditionData&  Condition;
	CGameSession&          Session;
	const CGameVarStorage* pVars = nullptr;
};

bool EvaluateCondition(const CConditionData& Cond, CGameSession& Session, const CGameVarStorage* pVars);
std::string GetConditionText(const CConditionData& Cond, CGameSession& Session, const CGameVarStorage* pVars);
HEntity ResolveEntityID(const Data::PParams& Params, CStrID ParamID, const CGameVarStorage* pVars);

using FEventCallback = std::function<void(std::unique_ptr<CGameVarStorage>&)>;

class ICondition
{
public:

	virtual ~ICondition() = default;

	virtual bool Evaluate(const CConditionContext& Ctx) const = 0;
	virtual void GetText(std::string& Out, const CConditionContext& Ctx) const {}
	virtual void SubscribeRelevantEvents(std::vector<Events::CConnection>& OutSubs, const CConditionContext& Ctx, const FEventCallback& Callback) const {}
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

template<> constexpr auto RegisterClassName<Game::CConditionData>() { return "DEM::Game::CConditionData"; }
template<> constexpr auto RegisterMembers<Game::CConditionData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Game::CConditionData, Type),
		DEM_META_MEMBER_FIELD(Game::CConditionData, Params)
	);
}

}
