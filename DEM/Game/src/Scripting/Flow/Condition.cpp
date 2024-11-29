#include "Condition.h"
#include <Scripting/Flow/ConditionRegistry.h>
#include <Game/SessionVars.h>
#include <Game/GameSession.h>

namespace DEM::Flow
{
static const CStrID sidLeft("Left");
static const CStrID sidOp("Op");
static const CStrID sidRight("Right");
static const CStrID sidOpLess("<");
static const CStrID sidOpLessEq("<=");
static const CStrID sidOpGreater(">");
static const CStrID sidOpGreaterEq(">=");
static const CStrID sidOpEq("==");
static const CStrID sidOpNeq("!=");
static const CStrID sidCode("Code");

template<typename T, typename U>
static inline bool Compare(const T& Left, CStrID Op, const U& Right)
{
	if (Op == sidOpEq)
	{
		if constexpr (Meta::is_equality_comparable<T, U>)
			return Left == Right;
		else
			return !(Left < Right || Right < Left);
	}
	else if (Op == sidOpNeq)
	{
		if constexpr (Meta::is_equality_comparable<T, U>)
			return !(Left == Right);
		else
			return Left < Right || Right < Left;
	}
	else if (Op == sidOpLess)
		return Left < Right;
	else if (Op == sidOpLessEq)
		return !(Right < Left);
	else if (Op == sidOpGreater)
		return Right < Left;
	else if (Op == sidOpGreaterEq)
		return !(Left < Right);
	else
		return false;
}
//---------------------------------------------------------------------

static inline bool CompareVarData(HVar Left, CStrID Op, const Data::CData& Right, const CFlowVarStorage& Vars)
{
	bool Result = false;
	Vars.Visit(Left, [&Right, Op, &Result](auto&& LeftValue)
	{
		using THRDType = Data::THRDType<std::decay_t<decltype(LeftValue)>>;
		if constexpr (Data::CTypeID<THRDType>::IsDeclared)
		{
			if (const THRDType* pRightValue = Right.As<THRDType>()) //???TODO: compare HRD null to monostate?!
			{
				if constexpr (std::is_same_v<THRDType, CString>)
					Result = Compare(LeftValue, Op, std::string_view{ *pRightValue });
				else
					Result = Compare(LeftValue, Op, *pRightValue);
			}
		}
	});

	return Result;
}
//---------------------------------------------------------------------

static inline bool CompareVarVar(HVar Left, CStrID Op, HVar Right, const CFlowVarStorage& LeftVars, const CFlowVarStorage& RightVars)
{
	bool Result = false;
	LeftVars.Visit(Left, [&RightVars, Right, Op, &Result](auto&& LeftValue)
	{
		RightVars.Visit(Right, [&LeftValue, Op, &Result](auto&& RightValue)
		{
			using TLeft = std::decay_t<decltype(LeftValue)>;
			using TRight = std::decay_t<decltype(RightValue)>;
			if constexpr (std::is_same_v<TLeft, TRight>) // TODO: can add is_convertible_v but not for bools, there are errors and warnings for them!
				Result = Compare(LeftValue, Op, RightValue);
		});
	});

	return Result;
}
//---------------------------------------------------------------------

static std::pair<const CFlowVarStorage*, HVar> FindVar(Game::CGameSession& Session, const CFlowVarStorage& Vars, CStrID ID)
{
	auto Handle = Vars.Find(ID);
	if (Handle) return { &Vars, Handle };

	if (const auto* pSessionVars = Session.FindFeature<Game::CSessionVars>())
	{
		Handle = pSessionVars->Runtime.Find(ID);
		if (Handle) return { &pSessionVars->Runtime, Handle };

		Handle = pSessionVars->Persistent.Find(ID);
		if (Handle) return { &pSessionVars->Persistent, Handle };
	}

	return { nullptr, {} };
}
//---------------------------------------------------------------------

bool EvaluateCondition(const CConditionData& Cond, Game::CGameSession& Session, const CFlowVarStorage& Vars)
{
	if (!Cond.Type) return true;

	const auto* pConditions = Session.FindFeature<CConditionRegistry>();
	if (!pConditions) return true;

	if (auto* pCondition = pConditions->FindCondition(Cond.Type))
		return pCondition->Evaluate(Cond.Params.Get(), Session, Vars);

	::Sys::Error("Unsupported condition type");
	return false;
}
//---------------------------------------------------------------------

bool CAndCondition::Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	if (!Params) return true;

	CConditionData Inner;
	for (const auto& Param : *Params)
	{
		DEM::ParamsFormat::Deserialize(Param.GetRawValue(), Inner);
		if (!EvaluateCondition(Inner, Session, Vars)) return false;
	}
	return true;
}
//---------------------------------------------------------------------

bool COrCondition::Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	if (!Params) return true;

	CConditionData Inner;
	for (const auto& Param : *Params)
	{
		DEM::ParamsFormat::Deserialize(Param.GetRawValue(), Inner);
		if (EvaluateCondition(Inner, Session, Vars)) return true;
	}
	return false;
}
//---------------------------------------------------------------------

bool CNotCondition::Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	if (!Params) return true;

	CConditionData Inner;
	DEM::ParamsFormat::Deserialize(Params, Inner);
	return !EvaluateCondition(Inner, Session, Vars);
}
//---------------------------------------------------------------------

bool CVarCmpConstCondition::Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	if (!Params) return true;

	const auto [pLeftVars, Left] = FindVar(Session, Vars, Params->Get<CStrID>(sidLeft));
	if (!Left) return false;

	const auto* pRightParam = Params->Find(sidRight);
	return pRightParam && CompareVarData(Left, Params->Get<CStrID>(sidOp), pRightParam->GetRawValue(), *pLeftVars);
}
//---------------------------------------------------------------------

bool CVarCmpVarCondition::Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	if (!Params) return true;

	const auto [pLeftVars, Left] = FindVar(Session, Vars, Params->Get<CStrID>(sidLeft));
	if (!Left) return false;

	const auto [pRightVars, Right] = FindVar(Session, Vars, Params->Get<CStrID>(sidRight));
	return Right && CompareVarVar(Left, Params->Get<CStrID>(sidOp), Right, *pLeftVars, *pRightVars);
}
//---------------------------------------------------------------------

bool CLuaStringCondition::Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	if (!Params) return true;

	const std::string_view Code = Params->Get<CString>(sidCode, CString::Empty);
	if (Code.empty()) return true;

	sol::environment Env(Session.GetScriptState(), sol::create, Session.GetScriptState().globals());
	Env["Vars"] = &Vars;
	auto Result = Session.GetScriptState().script("return " + std::string(Code), Env);
	if (!Result.valid())
	{
		::Sys::Error(Result.get<sol::error>().what());
		return false;
	}

	//???SOL: why nil can't be negated? https://www.lua.org/pil/3.3.html
	const auto Type = Result.get_type();
	return (Type != sol::type::none && Type != sol::type::nil && Result);
}
//---------------------------------------------------------------------

}
