#include "FlowPlayer.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Game/GameSession.h>
#include <Core/Factory.h>

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
		return Left == Right;
	else if (Op == sidOpNeq)
		return Left != Right;
	else if (Op == sidOpLess)
		return Left < Right;
	else if (Op == sidOpLessEq)
		return Left <= Right;
	else if (Op == sidOpGreater)
		return Left > Right;
	else if (Op == sidOpGreaterEq)
		return Left >= Right;
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

static inline bool CompareVarVar(HVar Left, CStrID Op, HVar Right, const CFlowVarStorage& Vars)
{
	bool Result = false;
	Vars.Visit(Left, [&Vars, Right, Op, &Result](auto&& LeftValue)
	{
		Vars.Visit(Right, [&LeftValue, Op, &Result](auto&& RightValue)
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

bool EvaluateCondition(const CConditionData& Cond, Game::CGameSession& Session, const CFlowVarStorage& Vars)
{
	if (!Cond.Type) return true;

	if (Cond.Type == sidFalse) return false;

	if (Cond.Type == sidVarCmpConst || Cond.Type == sidVarCmpVar)
	{
		const CStrID Op = Cond.Params->Get<CStrID>(sidOp);

		const auto Left = Vars.Find(Cond.Params->Get<CStrID>(sidLeft));
		if (!Left) return false;

		if (Cond.Type == sidVarCmpConst)
		{
			const auto* pRightParam = Cond.Params->Find(sidRight);
			return pRightParam && CompareVarData(Left, Op, pRightParam->GetRawValue(), Vars);
		}
		else
		{
			const auto Right = Vars.Find(Cond.Params->Get<CStrID>(sidRight));
			return CompareVarVar(Left, Op, Right, Vars);
		}
	}
	else if (Cond.Type == sidLuaString)
	{
		const std::string_view Code = Cond.Params->Get<CString>(sidCode, CString::Empty);
		if (Code.empty()) return true;

		auto LuaFn = Session.GetScriptState().load("local Vars = ...; return " + std::string(Code));
		if (!LuaFn.valid())
		{
			::Sys::Error(LuaFn.get<sol::error>().what());
			return false;
		}

		auto Result = LuaFn.get<sol::function>()(Vars);
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

void IFlowAction::Continue(CUpdateContext& Ctx)
{
	Ctx.Error.clear();
	Ctx.Finished = false;
}
//---------------------------------------------------------------------

void IFlowAction::Break(CUpdateContext& Ctx)
{
	Ctx.NextActionID = EmptyActionID;
	Ctx.Error.clear();
	Ctx.Finished = true;
}
//---------------------------------------------------------------------

void IFlowAction::Throw(CUpdateContext& Ctx, std::string&& Error, bool CanRetry)
{
	Ctx.Error = std::move(Error);
	Ctx.Finished = !CanRetry;
}
//---------------------------------------------------------------------

void IFlowAction::Goto(CUpdateContext& Ctx, const CFlowLink* pLink, float consumedDt)
{
	Ctx.Error.clear();
	Ctx.Finished = true;
	if (pLink)
	{
		Ctx.dt = std::max(0.f, Ctx.dt - consumedDt);
		Ctx.NextActionID = pLink->DestID;
		Ctx.YieldToNextFrame = pLink->YieldToNextFrame;
	}
	else
	{
		// Equal to Break()
		Ctx.NextActionID = EmptyActionID;
	}
}
//---------------------------------------------------------------------

//???return ptr or index or both?
const CFlowLink* IFlowAction::GetFirstValidLink(Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	const CFlowLink* pResult = nullptr;
	ForEachValidLink(Session, Vars, [&pResult](size_t i, const CFlowLink& Link)
	{
		pResult = &Link;
		return false;
	});

	return pResult;
}
//---------------------------------------------------------------------

CFlowPlayer::~CFlowPlayer()
{
	Stop();
}
//---------------------------------------------------------------------

bool CFlowPlayer::Start(PFlowAsset Asset, U32 StartActionID)
{
	Stop();

	_Asset = std::move(Asset);
	if (!_Asset) return false;

	if (StartActionID == EmptyActionID)
	{
		StartActionID = _Asset->GetDefaultStartActionID();
		if (StartActionID == EmptyActionID) return false;
	}

	_NextActionID = StartActionID;
	_VarStorage = _Asset->GetDefaultVarStorage();

	OnStart();

	return true;
}
//---------------------------------------------------------------------

void CFlowPlayer::Stop()
{
	if (!IsPlaying())
		return;

	if (_CurrAction)
		_CurrAction->OnCancel();

	Finish(false);
}
//---------------------------------------------------------------------

void CFlowPlayer::Update(Game::CGameSession& Session, float dt)
{
	// Resume from yielding or start the first action
	if (_NextActionID != EmptyActionID)
	{
		SetCurrentAction(Session, _NextActionID);
		_NextActionID = EmptyActionID;
	}

	if (!_CurrAction) return;

	// The context is filled for IFlowAction::Continue() by default. Actions that
	// don't control the flow explicitly will continue executing at the next frame.
	CUpdateContext Ctx;
	Ctx.pSession = &Session;
	Ctx.dt = dt;
	do
	{
		_CurrAction->Update(Ctx);

		// Print both critical and retryable errors
		const bool HasError = !Ctx.Error.empty();
		if (HasError)
			::Sys::Log(("[DEM.Flow] Error: " + Ctx.Error + "\n").c_str());

		// If the action must be continued, we will do it at the next frame
		if (!Ctx.Finished)
			break;

		// End the script when the last action is reached or a critical error is thrown
		if (HasError || Ctx.NextActionID == EmptyActionID)
		{
			Finish(HasError);
			break;
		}

		// If the traversed link requests yielding, delay the next action start to the next frame
		if (Ctx.YieldToNextFrame)
		{
			_CurrAction = nullptr;
			_NextActionID = Ctx.NextActionID;
			break;
		}

		// Otherwise proceed to the new action with remaining part of dt
		SetCurrentAction(Session, Ctx.NextActionID);

		// Reset Ctx state to IFlowAction::Continue() for the correct default behaviour
		Ctx.Finished = false;
	}
	while (_CurrAction);
}
//---------------------------------------------------------------------

void CFlowPlayer::SetCurrentAction(Game::CGameSession& Session, U32 ID)
{
	_CurrAction = nullptr;

	if (auto* pActionData = _Asset->FindAction(ID))
	{
		_CurrAction.reset(Core::CFactory::Instance().Create<IFlowAction>(pActionData->ClassName.CStr()));
		if (_CurrAction)
		{
			_CurrAction->_pPrototype = pActionData;
			_CurrAction->_pPlayer = this;
			_CurrAction->OnStart(Session);
		}
	}

	n_assert_dbg(_CurrAction);
}
//---------------------------------------------------------------------

void CFlowPlayer::Finish(bool WithError)
{
	n_assert(!WithError);

	OnFinish(_CurrAction ? _CurrAction->_pPrototype->ID : _NextActionID, WithError);

	_CurrAction = nullptr;
}
//---------------------------------------------------------------------

}
