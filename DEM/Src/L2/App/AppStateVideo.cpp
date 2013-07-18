#include "AppStateVideo.h"

#include <Render/RenderServer.h>
#include <Time/TimeServer.h>
#include <Debug/DebugServer.h>
#include <Events/EventServer.h>
#include <Video/VideoServer.h>
#include <Input/InputServer.h>
#include <Core/CoreServer.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateVideo, App::CStateHandler);

void CAppStateVideo::OnStateEnter(CStrID PrevState, PParams Params)
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

	RenderSrv->GetDisplay().ProcessWindowMessages();

	//???AudioSrv?
	VideoSrv->Trigger();
	InputSrv->Trigger();

	if (!VideoSrv->IsPlaying()) ReturnState = NextState; // Video has finished

	//else ReturnState = EXIT_STATE; // Application closed

	CoreSrv->Trigger();

	return ReturnState;
}
//---------------------------------------------------------------------

} // namespace Application
