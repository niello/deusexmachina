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
	Ctx.NextActionID = EmptyActionID_FIXME;
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

CFlowPlayer::~CFlowPlayer() = default;
//---------------------------------------------------------------------

bool CFlowPlayer::Start(PFlowAsset Asset, U32 StartActionID)
{
	Stop();

	_Asset = Asset;
	if (!_Asset) return false;

	if (StartActionID == EmptyActionID)
	{
		StartActionID = Asset->GetDefaultStartActionID();
		if (StartActionID == EmptyActionID) return false;
	}

	SetCurrentAction(StartActionID);
	if (!_CurrAction) return false;

	// TODO:
	//Variable storage is initialized with default values from the asset.
	//Global OnStart is fired/invoked. Need? Start is always controlled by external logic!

	return true;
}
//---------------------------------------------------------------------

void CFlowPlayer::Stop()
{
	if (_CurrAction)
		_CurrAction->OnCancel();

	Finish(false);
}
//---------------------------------------------------------------------

void CFlowPlayer::Update(float dt)
{
	// Resume from yielding
	if (_NextActionID != EmptyActionID)
	{
		SetCurrentAction(_NextActionID);
		_NextActionID = EmptyActionID;
	}

	if (!_CurrAction) return;

	CUpdateContext Ctx;
	Ctx.dt = dt;
	do
	{
		_CurrAction->Update(Ctx); ///TODO: also pass player! and maybe session?!

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
			_CurrAction->OnStart();
	}

	n_assert_dbg(_CurrAction);
}
//---------------------------------------------------------------------

void CFlowPlayer::Finish(bool WithError)
{
	n_assert(!WithError);

	_CurrAction = nullptr;
	//fire global OnFinish, possibly with the previous action ID and WithError flag
}
//---------------------------------------------------------------------

}
