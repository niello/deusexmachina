#include "FlowPlayer.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::Flow
{
static const CStrID sidVarCmpConst("VarCmpConst");
static const CStrID sidVarCmpVar("VarCmpVar");
static const CStrID sidLeft("Left");
static const CStrID sidOp("Op");
static const CStrID sidRight("Right");
static const CStrID sidOpLess("<");
static const CStrID sidOpLessEq("<=");
static const CStrID sidOpGreater(">");
static const CStrID sidOpGreaterEq(">=");
static const CStrID sidOpEq("==");
static const CStrID sidOpNeq("!=");

template<typename T, typename U>
static inline bool Compare(T&& Left, CStrID Op, U&& Right)
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
	if (!Left || !Right || Left.TypeIdx != Right.TypeIdx) return false;

	const auto LeftValue = Vars.Get(Left);
	const auto RightValue = Vars.Get(Right);
	return Compare(LeftValue, Op, RightValue);
}
//---------------------------------------------------------------------

bool EvaluateCondition(const CConditionData& Cond, const Game::CGameSession& Session, const CFlowVarStorage& Vars)
{
	if (!Cond.Type) return true;

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
const CFlowLink* IFlowAction::GetFirstValidLink(const Game::CGameSession& Session, const CFlowVarStorage& Vars) const
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
