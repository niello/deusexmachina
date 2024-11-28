#include "Condition.h"
#include <Scripting/Flow/ConditionRegistry.h>
#include <Game/SessionVars.h>
#include <Game/GameSession.h>

//!!!DBG TMP!
// conditions need no factory and 'new', they are stateless and registered by type IDs (CStrID or enum), returning a 'singleton' instance of cond type.
// Lua condition stores sol::function objects but this is code, not state. Session will be needed for that.
// Lua condition gets script as input and loads functions from it by names, without putting them to the global namespace!
// Registration requires a pair ID -> instance (or template type and variadic args for construction).
// Condition registry may live in a session, it is needed for Lua anyway. Add registry as a feature?
// No refcounting needed because of singleton nature of conditions. unique_ptr is enough. No RTTI is needed because there is no factory.
// Data::PParams arg is always provided into evaluation, because it is a state
// Unordered map should be used for fast condition lookup, because composite conditions will have to do lookup where now is DEM::ParamsFormat::Deserialize.

namespace DEM::Flow
{
static const CStrID sidAnd("And");
static const CStrID sidOr("Or");
static const CStrID sidNot("Not");
static const CStrID sidFalse("False");
static const CStrID sidVarCmpConst("VarCmpConst");
static const CStrID sidVarCmpVar("VarCmpVar");
static const CStrID sidLuaString("LuaString");
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

	if (Cond.Type == sidFalse) return false;

	if (Cond.Type == sidVarCmpConst || Cond.Type == sidVarCmpVar)
	{
		const CStrID Op = Cond.Params->Get<CStrID>(sidOp);

		const auto [ pLeftVars, Left ] = FindVar(Session, Vars, Cond.Params->Get<CStrID>(sidLeft));
		if (!Left) return false;

		if (Cond.Type == sidVarCmpConst)
		{
			const auto* pRightParam = Cond.Params->Find(sidRight);
			return pRightParam && CompareVarData(Left, Op, pRightParam->GetRawValue(), *pLeftVars);
		}
		else
		{
			const auto [pRightVars, Right] = FindVar(Session, Vars, Cond.Params->Get<CStrID>(sidRight));
			return Right && CompareVarVar(Left, Op, Right, *pLeftVars, *pRightVars);
		}
	}
	else if (Cond.Type == sidLuaString)
	{
		const std::string_view Code = Cond.Params->Get<CString>(sidCode, CString::Empty);
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
	else if (Cond.Type == sidAnd)
	{
		CConditionData Inner;
		for (const auto& Param : *Cond.Params)
		{
			DEM::ParamsFormat::Deserialize(Param.GetRawValue(), Inner);
			if (!EvaluateCondition(Inner, Session, Vars)) return false;
		}
		return true;
	}
	else if (Cond.Type == sidOr)
	{
		CConditionData Inner;
		for (const auto& Param : *Cond.Params)
		{
			DEM::ParamsFormat::Deserialize(Param.GetRawValue(), Inner);
			if (EvaluateCondition(Inner, Session, Vars)) return true;
		}
		return false;
	}
	else if (Cond.Type == sidNot)
	{
		CConditionData Inner;
		DEM::ParamsFormat::Deserialize(Cond.Params, Inner);
		return !EvaluateCondition(Inner, Session, Vars);
	}

	::Sys::Error("Unsupported condition type");
	return false;
}
//---------------------------------------------------------------------

}
