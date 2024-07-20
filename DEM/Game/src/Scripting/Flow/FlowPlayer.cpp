#include "FlowPlayer.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::Flow
{

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

void IFlowAction::Goto(CUpdateContext& Ctx, const CFlowLink& Link, float consumedDt)
{
	Ctx.dt = std::max(0.f, Ctx.dt - consumedDt);
	Ctx.NextActionID = Link.DestID;
	Ctx.Error.clear();
	Ctx.Finished = true;
	Ctx.YieldToNextFrame = Link.YieldToNextFrame;
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

	_VarStorage = _Asset->GetDefaultVarStorage();

	// Before SetCurrentAction() to give a caller chance to fill variables externally
	OnStart();

	SetCurrentAction(StartActionID);
	return !!_CurrAction;
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
	// Resume from yielding
	if (_NextActionID != EmptyActionID)
	{
		SetCurrentAction(_NextActionID);
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
			::Sys::Log(Ctx.Error.c_str());

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
		SetCurrentAction(Ctx.NextActionID);

		// Reset Ctx state to IFlowAction::Continue() for the correct default behaviour
		Ctx.Finished = false;
	}
	while (_CurrAction);
}
//---------------------------------------------------------------------

void CFlowPlayer::SetCurrentAction(U32 ID)
{
	_CurrAction = nullptr;

	if (auto* pActionData = _Asset->FindAction(ID))
	{
		_CurrAction.reset(Core::CFactory::Instance().Create<IFlowAction>(pActionData->ClassName.CStr()));
		if (_CurrAction)
		{
			_CurrAction->_pPrototype = pActionData;
			_CurrAction->_pPlayer = this;
			_CurrAction->OnStart();
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
