#include "FlowPlayer.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::Flow
{

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

	Finish();
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

		if (!Ctx.Error.empty())
			::Sys::Log(Ctx.Error.c_str());

		if (Ctx.Finished)
		{
			if (Ctx.NextActionID != EmptyActionID)
			{
				if (Ctx.YieldToNextFrame)
				{
					_CurrAction = nullptr;
					_NextActionID = Ctx.NextActionID;
					break;
				}
				else
				{
					SetCurrentAction(Ctx.NextActionID);
				}
			}
			else
			{
				Finish();
				break;
			}
		}
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

void CFlowPlayer::Finish()
{
	_CurrAction = nullptr;
	//fire global OnFinish, possibly with the previous action ID and result type (empty pin requested = finish, or error).
}
//---------------------------------------------------------------------

}
