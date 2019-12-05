#include "AppStateVideo.h"

#include <Core/CoreServer.h>
#include <Debug/DebugServer.h>
#include <Events/EventServer.h>
#include <Video/VideoServer.h>
#include <Core/CoreServer.h>

namespace App
{
RTTI_CLASS_IMPL(App::CAppStateVideo, App::CStateHandler);

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
	DbgSrv->Trigger();
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