#include "AppStateVideo.h"

#include <Core/CoreServer.h>
#include <Events/EventServer.h>
#include <Video/VideoServer.h>

namespace App
{

void CAppStateVideo::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	CoreSrv->ResetAll();
	CoreSrv->Trigger();
	CStateHandler::OnStateEnter(PrevState);
	n_assert(VideoFileName.IsValid());
	VideoSrv->ScalingEnabled = EnableScaling;
	VideoSrv->PlayFile(VideoFileName.CStr());
}
//---------------------------------------------------------------------

CStrID CAppStateVideo::OnFrame()
{
	CStrID ReturnState = ID;

	CoreSrv->Trigger();
	EventSrv->ProcessPendingEvents();

//	RenderSrv->GetDisplay().ProcessWindowMessages();

	//???AudioSrv?
	VideoSrv->Trigger();

	if (!VideoSrv->IsPlaying()) ReturnState = NextState; // Video has finished

	//else ReturnState = EXIT_STATE; // Application closed

	return ReturnState;
}
//---------------------------------------------------------------------

}
