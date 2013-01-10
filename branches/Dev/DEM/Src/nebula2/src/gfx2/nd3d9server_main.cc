//------------------------------------------------------------------------------
//  nd3d9server_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9server.h"
#include <Data/DataServer.h>
#include "gfx2/nd3d9texture.h"
#include <Events/EventManager.h>
#include <Gfx/Events/DisplayInput.h>
#include <Render/RenderServer.h>

nD3D9Server* nD3D9Server::Singleton = 0;

nD3D9Server::nD3D9Server():
	deviceBehaviourFlags(0),
	pD3DXSprite(NULL),
	pD3DFont(NULL),
	pD3D9(0),
	pD3D9Device(0),
	depthStencilSurface(0),
	backBufferSurface(0),
	pEffectPool(0),
	featureSet(InvalidFeatureSet),
	textElements(64, 64),
#if __NEBULA_STATS__
	timeStamp(0.0),
	queryResourceManager(0),
	statsFrameCount(0),
	statsNumTextureChanges(0),
	statsNumRenderStateChanges(0),
	statsNumDrawCalls(0),
	statsNumPrimitives(0),
#endif
	d3dxLine(0)
{
	n_assert(!Singleton);
	Singleton = this;

	memset(&devCaps, 0, sizeof(devCaps));
	memset(&presentParams, 0, sizeof(presentParams));
	memset(&shapeMeshes, 0, sizeof(shapeMeshes));
}
//---------------------------------------------------------------------

nD3D9Server::~nD3D9Server()
{
	if (displayOpen) CloseDisplay();
	n_assert(Singleton);
	Singleton = NULL;
}
//---------------------------------------------------------------------

const CDisplayMode& nD3D9Server::GetDisplayMode() const
{
	return RenderSrv->GetDisplay().GetDisplayMode();
}
//---------------------------------------------------------------------

void nD3D9Server::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	RenderSrv->GetDisplay().GetRelativeXY(XAbs, YAbs, XRel, YRel);
}
//---------------------------------------------------------------------

HWND nD3D9Server::GetAppHwnd() const
{
    return RenderSrv->GetDisplay().GetAppHwnd();
}
//---------------------------------------------------------------------

HWND nD3D9Server::GetParentHwnd() const
{
    return RenderSrv->GetDisplay().GetParentHwnd();
}
//---------------------------------------------------------------------

bool nD3D9Server::OpenDisplay()
{
	n_assert(!displayOpen);

	SUBSCRIBE_PEVENT(OnDisplaySetCursor, nD3D9Server, OnSetCursor);
	SUBSCRIBE_PEVENT(OnDisplayPaint, nD3D9Server, OnPaint);
	SUBSCRIBE_PEVENT(OnDisplayToggleFullscreen, nD3D9Server, OnToggleFullscreenWindowed);
	SUBSCRIBE_NEVENT(DisplayInput, nD3D9Server, OnDisplayInput);

	if (!DeviceOpen()) FAIL;
	nGfxServer2::OpenDisplay();

	ClearScreen(0xff000000, 1.0f, 0);

	OK;
}
//---------------------------------------------------------------------

void nD3D9Server::CloseDisplay()
{
	n_assert(displayOpen);

	UNSUBSCRIBE_EVENT(OnDisplaySetCursor);
	UNSUBSCRIBE_EVENT(OnDisplayPaint);
	UNSUBSCRIBE_EVENT(OnDisplayToggleFullscreen);
	UNSUBSCRIBE_EVENT(DisplayInput);

	DeviceClose();
	nGfxServer2::CloseDisplay();
}
//---------------------------------------------------------------------

// Implements the Windows message pump. Must be called once a frame OUTSIDE of BeginScene() / EndScene().
void nD3D9Server::Trigger()
{
	RenderSrv->GetDisplay().ProcessWindowMessages();
}
//---------------------------------------------------------------------

void nD3D9Server::EnterDialogBoxMode()
{
	n_assert(pD3D9Device);
	HRESULT hr;

	nGfxServer2::EnterDialogBoxMode();

	// reset the device with lockable backbuffer flag
	OnDeviceCleanup(false);
	D3DPRESENT_PARAMETERS p = presentParams;
	p.MultiSampleType = D3DMULTISAMPLE_NONE;
	p.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	hr = pD3D9Device->Reset(&p);
	InitDeviceState();
	OnDeviceInit(false);

	pD3D9Device->SetDialogBoxMode(TRUE);
}
//---------------------------------------------------------------------

void nD3D9Server::LeaveDialogBoxMode()
{
	n_assert(pD3D9Device);
	nGfxServer2::LeaveDialogBoxMode();
	pD3D9Device->SetDialogBoxMode(FALSE);

	// only reset the device if it is currently valid
	if (SUCCEEDED(pD3D9Device->TestCooperativeLevel()))
	{
		OnDeviceCleanup(false);
		pD3D9Device->Reset(&presentParams);
		InitDeviceState();
		OnDeviceInit(false);
	}
}
//---------------------------------------------------------------------

bool nD3D9Server::OnSetCursor(const Events::CEventBase& Event)
{
	if (!pD3D9Device) FAIL;

	switch (cursorVisibility)
	{
		case nGfxServer2::None:
		case nGfxServer2::Gui:
			SetCursor(NULL);
			pD3D9Device->ShowCursor(FALSE);
			OK;

		case nGfxServer2::System:
			pD3D9Device->ShowCursor(FALSE);
			FAIL;

		case nGfxServer2::Custom:
			SetCursor(NULL);
			pD3D9Device->ShowCursor(TRUE);
			OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool nD3D9Server::OnPaint(const Events::CEventBase& Event)
{
	if (RenderSrv->GetDisplay().Fullscreen && pD3D9Device && !inDialogBoxMode)
		pD3D9Device->Present(0, 0, 0, 0);
	OK;
}
//---------------------------------------------------------------------

bool nD3D9Server::OnToggleFullscreenWindowed(const Events::CEventBase& Event)
{
	RenderSrv->GetDisplay().Fullscreen = !RenderSrv->GetDisplay().Fullscreen;
	CloseDisplay();
	OpenDisplay();
	OK;
}
//---------------------------------------------------------------------

// In full-screen mode, update the cursor position myself
bool nD3D9Server::OnDisplayInput(const Events::CEventBase& Event)
{
	const Event::DisplayInput& Ev = (const Event::DisplayInput&)Event;

	if (RenderSrv->GetDisplay().Fullscreen && Ev.Type == Event::DisplayInput::MouseMove)
		pD3D9Device->SetCursorPosition(Ev.MouseInfo.x, Ev.MouseInfo.y, 0);
	OK;
}
//---------------------------------------------------------------------
