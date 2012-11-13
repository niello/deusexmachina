#include "PlayVideoHandler.h"

#include <Time/TimeServer.h>
#include <Debug/DebugServer.h>
#include <Events/EventManager.h>
#include <Video/VideoServer.h>
#include <Input/InputServer.h>
#include <gfx2/ngfxserver2.h>

namespace App
{
ImplementRTTI(App::CPlayVideoHandler, App::CStateHandler);
//ImplementFactory(App::CPlayVideoHandler);

void CPlayVideoHandler::OnStateEnter(CStrID PrevState, PParams Params)
{
	TimeSrv->ResetAll();
	TimeSrv->Update();
	CStateHandler::OnStateEnter(PrevState);
	n_assert(VideoFileName.IsValid());
	VideoSrv->ScalingEnabled = EnableScaling;
	VideoSrv->PlayFile(VideoFileName.Get());
}
//---------------------------------------------------------------------

CStrID CPlayVideoHandler::OnFrame()
{
	CStrID ReturnState = ID;

	TimeSrv->Update();
	DbgSrv->Trigger();
	EventMgr->ProcessPendingEvents();

	nGfxServer2::Instance()->Trigger();

	VideoSrv->Trigger();
	InputSrv->Trigger();

	if (!VideoSrv->IsPlaying()) ReturnState = NextState; // Video has finished

	//else ReturnState = EXIT_STATE; // Application closed

	//!!!to some method of memory/core server!
	nMemoryStats memStats = n_dbgmemgetstats();
	CoreSrv->SetGlobal<int>("MemHighWaterSize", memStats.highWaterSize);
	CoreSrv->SetGlobal<int>("MemTotalSize", memStats.totalSize);
	CoreSrv->SetGlobal<int>("MemTotalCount", memStats.totalCount);

	return ReturnState;
}
//---------------------------------------------------------------------

} // namespace Application
