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

static inline bool CompareVarData(HVar Left, CStrID Op, const Data::CData& Right, const CBasicVarStorage& Vars)
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

static inline bool CompareVarVar(HVar Left, CStrID Op, HVar Right, const CBasicVarStorage& LeftVars, const CBasicVarStorage& RightVars)
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

static std::pair<const CBasicVarStorage*, HVar> FindVar(Game::CGameSession& Session, const CBasicVarStorage* pVars, CStrID ID)
{
	if (pVars)
	{
		auto Handle = pVars->Find(ID);
		if (Handle) return { pVars, Handle };
	}

	if (const auto* pSessionVars = Session.FindFeature<Game::CSessionVars>())
	{
		auto Handle = pSessionVars->Runtime.Find(ID);
		if (Handle) return { &pSessionVars->Runtime, Handle };

		Handle = pSessionVars->Persistent.Find(ID);
		if (Handle) return { &pSessionVars->Persistent, Handle };
	}

	return { nullptr, {} };
}
//---------------------------------------------------------------------

bool EvaluateCondition(const CConditionData& Cond, Game::CGameSession& Session, const CBasicVarStorage* pVars)
{
	if (!Cond.Type) return true;

	const auto* pConditions = Session.FindFeature<CConditionRegistry>();
	if (!pConditions) return true;

	if (auto* pCondition = pConditions->FindCondition(Cond.Type))
		return pCondition->Evaluate({ Cond, Session, pVars });

	::Sys::Error("Unsupported condition type");
	return false;
}
//---------------------------------------------------------------------

std::string GetConditionText(const CConditionData& Cond, Game::CGameSession& Session, const CBasicVarStorage* pVars)
{
	std::string Result;

	if (Cond.Type)
		if (const auto* pConditions = Session.FindFeature<CConditionRegistry>())
			if (auto* pCondition = pConditions->FindCondition(Cond.Type))
				pCondition->GetText(Result, { Cond, Session, pVars });

	return Result;
}
//---------------------------------------------------------------------

Game::HEntity ResolveEntityID(const Data::PParams& Params, CStrID ParamID, const CBasicVarStorage* pVars)
{
	if (auto* pParam = Params->Find(ParamID))
	{
		if (pParam->IsA<int>())
		{
			// An entity ID is provided in an action parameter
			return Game::HEntity{ static_cast<DEM::Game::HEntity::TRawValue>(pParam->GetValue<int>()) };
		}
		else if (pVars && pParam->IsA<CStrID>())
		{
			// An entity ID is stored in a flow player variable storage and is referenced in action by var ID
			const int Raw = pVars->Get<int>(pVars->Find(pParam->GetValue<CStrID>()), static_cast<int>(Game::HEntity{}.Raw));
			return Game::HEntity{ static_cast<DEM::Game::HEntity::TRawValue>(Raw) };
		}
	}

	return {};
}
//---------------------------------------------------------------------

bool CAndCondition::Evaluate(const CConditionContext& Ctx) const
{
	const auto& Params = Ctx.Condition.Params;
	if (!Params) return true;

	for (const auto& Param : *Params)
	{
		CConditionData Inner;
		DEM::ParamsFormat::Deserialize(Param.GetRawValue(), Inner);
		if (!EvaluateCondition(Inner, Ctx.Session, Ctx.pVars)) return false;
	}
	return true;
}
//---------------------------------------------------------------------

bool COrCondition::Evaluate(const CConditionContext& Ctx) const
{
	const auto& Params = Ctx.Condition.Params;
	if (!Params) return true;

	for (const auto& Param : *Params)
	{
		CConditionData Inner;
		DEM::ParamsFormat::Deserialize(Param.GetRawValue(), Inner);
		if (EvaluateCondition(Inner, Ctx.Session, Ctx.pVars)) return true;
	}
	return false;
}
//---------------------------------------------------------------------

bool CNotCondition::Evaluate(const CConditionContext& Ctx) const
{
	const auto& Params = Ctx.Condition.Params;
	if (!Params) return true;

	CConditionData Inner;
	DEM::ParamsFormat::Deserialize(Params, Inner);
	return !EvaluateCondition(Inner, Ctx.Session, Ctx.pVars);
}
//---------------------------------------------------------------------

bool CVarCmpConstCondition::Evaluate(const CConditionContext& Ctx) const
{
	const auto& Params = Ctx.Condition.Params;
	if (!Params) return true;

	const auto [pLeftVars, Left] = FindVar(Ctx.Session, Ctx.pVars, Params->Get<CStrID>(sidLeft));
	if (!Left) return false;

	const auto* pRightParam = Params->Find(sidRight);
	return pRightParam && CompareVarData(Left, Params->Get<CStrID>(sidOp), pRightParam->GetRawValue(), *pLeftVars);
}
//---------------------------------------------------------------------

void CVarCmpConstCondition::GetText(std::string& Out, const CConditionContext& Ctx) const
{
	const auto& Params = Ctx.Condition.Params;
	if (!Params) return;

	Out.append(Params->Get<CStrID>(sidLeft).ToStringView());
	Out.append(Params->Get<CStrID>(sidOp).ToStringView());
	if (const auto* pRightParam = Params->Find(sidRight))
		Out.append(pRightParam->GetRawValue().ToString());
	else
		Out.append("<MISSING>");
}
//---------------------------------------------------------------------

bool CVarCmpVarCondition::Evaluate(const CConditionContext& Ctx) const
{
	const auto& Params = Ctx.Condition.Params;
	if (!Params) return true;

	const auto [pLeftVars, Left] = FindVar(Ctx.Session, Ctx.pVars, Params->Get<CStrID>(sidLeft));
	if (!Left) return false;

	const auto [pRightVars, Right] = FindVar(Ctx.Session, Ctx.pVars, Params->Get<CStrID>(sidRight));
	return Right && CompareVarVar(Left, Params->Get<CStrID>(sidOp), Right, *pLeftVars, *pRightVars);
}
//---------------------------------------------------------------------

bool CLuaStringCondition::Evaluate(const CConditionContext& Ctx) const
{
	const auto& Params = Ctx.Condition.Params;
	if (!Params) return true;

	const std::string_view Code = Params->Get<CString>(sidCode, CString::Empty);
	if (Code.empty()) return true;

	sol::environment Env(Ctx.Session.GetScriptState(), sol::create, Ctx.Session.GetScriptState().globals());
	Env["Vars"] = Ctx.pVars;
	auto Result = Ctx.Session.GetScriptState().script("return " + std::string(Code), Env);
	if (!Result.valid())
	{
		::Sys::Error(Result.get<sol::error>().what());
		return false;
	}

	//???SOL: why nil can't be negated? https://www.lua.org/pil/3.3.html
	const auto Type = Result.get_type();
	return (Type == sol::type::userdata) || (Type != sol::type::none && Type != sol::type::nil && Result);
}
//---------------------------------------------------------------------

}
