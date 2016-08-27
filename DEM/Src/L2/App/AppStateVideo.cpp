#include "AppStateVideo.h"

#include <Time/TimeServer.h>
#include <Debug/DebugServer.h>
#include <Events/EventServer.h>
#include <Video/VideoServer.h>
#include <Core/CoreServer.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateVideo, App::CStateHandler);

void CAppStateVideo::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	TimeSrv->ResetAll();
	TimeSrv->Trigger();
	CStateHandler::OnStateEnter(PrevState);
	n_assert(VideoFileName.IsValid());
	VideoSrv->ScalingEnabled = EnableScaling;
	VideoSrv->PlayFile(VideoFileName.CStr());
}
//---------------------------------------------------------------------

CStrID CAppStateVideo::OnFrame()
{
	CStrID ReturnState = ID;

	TimeSrv->Trigger();
	DbgSrv->Trigger();
	EventSrv->ProcessPendingEvents();

//	RenderSrv->GetDisplay().ProcessWindowMessages();

	//???AudioSrv?
	VideoSrv->Trigger();

	if (!VideoSrv->IsPlaying()) ReturnState = NextState; // Video has finished

	//else ReturnState = EXIT_STATE; // Application closed

	CoreSrv->Trigger();

	return ReturnState;
}
//---------------------------------------------------------------------

}