#include "VideoServer.h"

#include <Video/OGGTheoraPlayer.h>
#include <Time/TimeServer.h>
#include <Data/DataServer.h>
#include <gfx2/ngfxserver2.h>

namespace Video
{
ImplementRTTI(Video::CVideoServer, Core::CRefCounted);
__ImplementSingleton(Video::CVideoServer);

CVideoServer::CVideoServer():
	_IsOpen(false),
	_IsPlaying(false),
	ScalingEnabled(false),
	pGraphBuilder(0),
	pMediaCtl(0),
	pMediaEvent(0),
	pVideoWnd(0),
	pBasicVideo(0)
{
	__ConstructSingleton;
	CoInitialize(0);
}
//---------------------------------------------------------------------

CVideoServer::~CVideoServer()
{
	n_assert(!_IsPlaying);
	if (_IsOpen) Close();
	CoUninitialize();
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CVideoServer::Open()
{
	n_assert(!_IsOpen);
	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CVideoServer::Close()
{
	n_assert(_IsOpen);
	while (Players.Size() > 0) DeleteVideoPlayer(Players.Back());
	if (_IsPlaying) Stop();
	_IsOpen = false;
}
//---------------------------------------------------------------------

void CVideoServer::Trigger()
{
	n_assert(_IsOpen);

	float FrameTime = (float)TimeSrv->GetFrameTime();

	// Rewind players on time reset
	// Now can't happen due to the wrapping in the TimeSrv
	if (FrameTime < 0.f)
		for (int i = 0; i < Players.Size(); ++i)
			if (Players[i]->IsOpen()) Players[i]->Rewind();

	//???else? see right above.
	for (int i = 0; i < Players.Size(); ++i)
		if (Players[i]->IsOpen()) Players[i]->Decode(FrameTime);

	if (_IsPlaying)
	{
		n_assert(pMediaEvent);
		long EventCode;
		LONG_PTR Param1, Param2;
		while (pMediaEvent->GetEvent(&EventCode, &Param1, &Param2, 0) == S_OK)
			if (EventCode == EC_COMPLETE)
			{
				Stop();
				break;
			}
	}
}
//---------------------------------------------------------------------

bool CVideoServer::PlayFile(const char* pFileName)
{
	n_assert(pFileName && !pGraphBuilder && !pMediaCtl && !pVideoWnd && !pMediaEvent && !pBasicVideo);
	if (_IsPlaying) Stop();

	nGfxServer2::Instance()->ClearScreen(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0);
	nGfxServer2::Instance()->EnterDialogBoxMode();
	nGfxServer2::Instance()->ClearScreen(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0);

	if (FAILED(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraphBuilder)))
	{
		n_error("CVideoServer: could not create DirectShow filter graph!");
		FAIL;
	}

	wchar_t WidePath[N_MAXPATH];
	nString Path = DataSrv->ManglePath(pFileName);
	mbstowcs(WidePath, Path.Get(), Path.Length() + 1);

	if (FAILED(pGraphBuilder->RenderFile(WidePath, NULL)))
	{
		n_error("CVideoServer::PlayFile(): could not render file '%s'", Path.Get());
		FAIL;
	}

	n_assert(SUCCEEDED(pGraphBuilder->QueryInterface(IID_IMediaControl, (void**)&pMediaCtl)));
	n_assert(SUCCEEDED(pGraphBuilder->QueryInterface(IID_IMediaEvent, (void**)&pMediaEvent)));
	n_assert(SUCCEEDED(pGraphBuilder->QueryInterface(IID_IVideoWindow, (void**)&pVideoWnd)));
	pGraphBuilder->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);

	OAHWND OwnerHWnd = NULL;
	CoreSrv->GetGlobal("hwnd", (int&)OwnerHWnd);
	RECT Rect;
	GetClientRect((HWND)OwnerHWnd, &Rect);
	LONG VideoLeft, VideoTop, VideoWidth, VideoHeight;
	if (ScalingEnabled)
	{
		// Fullscreen
		VideoLeft = 0;
		VideoTop = 0;
		VideoWidth = Rect.right;
		VideoHeight = Rect.bottom;
	}
	else
	{
		// Render Video in original size, centered
		n_assert(SUCCEEDED(pBasicVideo->GetVideoSize(&VideoWidth, &VideoHeight)));
		VideoLeft = (Rect.right + Rect.left - VideoWidth) / 2;
		VideoTop = (Rect.bottom + Rect.top - VideoHeight) / 2;
	}

	// Setup Video window
	n_assert(SUCCEEDED(pVideoWnd->put_AutoShow(OATRUE)));
	n_assert(SUCCEEDED(pVideoWnd->put_Owner(OwnerHWnd)));
	n_assert(SUCCEEDED(pVideoWnd->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)));
	n_assert(SUCCEEDED(pVideoWnd->SetWindowPosition(VideoLeft, VideoTop, VideoWidth, VideoHeight)));
	n_assert(SUCCEEDED(pVideoWnd->put_MessageDrain(OwnerHWnd)));
	n_assert(SUCCEEDED(pVideoWnd->HideCursor(OATRUE)));

	n_assert(SUCCEEDED(pMediaCtl->Run()));

	_IsPlaying = true;
	OK;
}
//---------------------------------------------------------------------

void CVideoServer::Stop()
{
    n_assert(_IsPlaying);

	nGfxServer2::Instance()->LeaveDialogBoxMode();
	SAFE_RELEASE(pBasicVideo);
	SAFE_RELEASE(pVideoWnd);
	SAFE_RELEASE(pMediaCtl);
	SAFE_RELEASE(pMediaEvent);
	SAFE_RELEASE(pGraphBuilder);

	HWND hWnd = NULL;
	CoreSrv->GetGlobal("hwnd", (int&)hWnd);
	ShowWindow(hWnd, SW_RESTORE);

    _IsPlaying = false;
}
//---------------------------------------------------------------------

CVideoPlayer* CVideoServer::NewVideoPlayer(const nString& Name)
{
	CVideoPlayer* pPlayer = n_new(COGGTheoraPlayer);
	pPlayer->SetFilename(Name);
	Players.PushBack(pPlayer);
	return pPlayer;
}
//---------------------------------------------------------------------

void CVideoServer::DeleteVideoPlayer(CVideoPlayer* pPlayer)
{
	n_assert(pPlayer);
	for (int i = 0; i < Players.Size(); i++)
		if (Players[i] == pPlayer)
		{
			Players.Erase(i);
			break;
		}
	if (pPlayer->IsOpen()) pPlayer->Close();
	n_delete(pPlayer);
}
//---------------------------------------------------------------------

}