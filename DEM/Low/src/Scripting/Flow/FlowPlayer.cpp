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
	// If an action is running, cancel it. _NextActionID is non-empty only when an action has already finished.
	//???TODO: clear _CurrAction when setting _NextActionID? Need to take into account in IsPlaying / GetCurrentActionID, must return _NextActionID!
	if (_CurrAction && _NextActionID != EmptyActionID)
		_CurrAction->OnCancel();

	Finish();
}
//---------------------------------------------------------------------

void CFlowPlayer::Update(float dt)
{
	//1 if next action ID is not empty (yield), start the action, make it current and clear next ID field
	//2 if no curr action, return
	//3 if curr action active, update it with dt, passing the player instance in
	//4 if curr action has finished (see below)
	//	 if error or break (=no next action), finish script execution
	//	 destroy or return to cache an action C++ instance object
	//	 if yield to next frame, mark that we did it (by storing next action ID?) and return
	//	 start next action (i.e. the one at the chosen output pin)
	//	 make the next action current and goto 3
	//!!!TODO: must fail if no next action with specified ID exists!
}
//---------------------------------------------------------------------

void CFlowPlayer::SetCurrentAction(U32 ID)
{
	auto* pActionData = _Asset->FindAction(ID);
	if (!pActionData) return;
	_CurrAction.reset(Core::CFactory::Instance().Create<IFlowAction>(pActionData->ClassName.CStr()));
	if (!_CurrAction) return;
	_CurrAction->OnStart();
}
//---------------------------------------------------------------------

void CFlowPlayer::Finish()
{
	_CurrAction = nullptr;
	//This happens when the current action is empty or invalid, either at the playback start or after finishing a previous action.
	//fire global OnFinish, possibly with the previous action ID and result type
	//(empty pin requested = finish, or error).
}
//---------------------------------------------------------------------

}
