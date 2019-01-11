#include "Application.h"

#include <Events/EventServer.h>
#include <Events/Subscription.h>
#include <Math/Math.h>
#include <System/Platform.h>
#include <System/Win32/OSWindowWin32.h>
#include <Render/SwapChain.h>
#include <Render/GPUDriver.h>

namespace DEM { namespace Core
{

//???empty constructor, add Init to process init-time failures?
CApplication::CApplication(Sys::IPlatform& _Platform)
	: Platform(_Platform)
{
	// check multiple instances

	//???move RNG instance to an application instead of static vars? pass platform system time as seed?
	// RNG is initialized in constructor to be available anywhere
	Math::InitRandomNumberGenerator();

	// create default file system from platform
	// setup hard assigns from platform and application

	n_new(Events::CEventServer);
}
//---------------------------------------------------------------------

CApplication::~CApplication()
{
	if (Events::CEventServer::HasInstance()) n_delete(EventSrv);
}
//---------------------------------------------------------------------

// Creates a GUI window most suitable for 3D scene rendering, based on app & profile settings
int CApplication::CreateRenderWindow(Render::CGPUDriver* pGPU, U32 Width, U32 Height)
{
	auto Wnd = Platform.CreateGUIWindow();
	Wnd->SetRect(Data::CRect(50, 50, Width, Height));

	Render::CRenderTargetDesc BBDesc;
	BBDesc.Format = Render::PixelFmt_DefaultBackBuffer;
	BBDesc.MSAAQuality = Render::MSAA_None;
	BBDesc.UseAsShaderInput = false;
	BBDesc.MipLevels = 0;
	BBDesc.Width = 0;
	BBDesc.Height = 0;

	Render::CSwapChainDesc SCDesc;
	SCDesc.BackBufferCount = 2;
	SCDesc.SwapMode = Render::SwapMode_CopyDiscard;
	SCDesc.Flags = Render::SwapChain_AutoAdjustSize | Render::SwapChain_VSync;

	int SwapChainID = pGPU->CreateSwapChain(BBDesc, SCDesc, Wnd);
	n_assert(pGPU->SwapChainExists(SwapChainID));
	return SwapChainID;
}
//---------------------------------------------------------------------

//???need Run()? use Init()?
bool CApplication::Run(/*initial state?*/)
{
	BaseTime = Platform.GetSystemTime();
	PrevTime = BaseTime;
	FrameTime = 0.0;

	// initialize systems

	// start FSM initial state (enter state)
	OK;
}
//---------------------------------------------------------------------

bool CApplication::Update()
{
	// Update time

	constexpr double MAX_FRAME_TIME = 0.25;

	const double CurrTime = Platform.GetSystemTime();
	FrameTime = CurrTime - PrevTime;
	if (FrameTime < 0.0) FrameTime = 1.0 / 60.0;
	else if (FrameTime > MAX_FRAME_TIME) FrameTime = MAX_FRAME_TIME;
	FrameTime *= TimeScale;

	PrevTime = CurrTime;

	// Process OS messages etc

	Platform.Update();

	// ...

	// Update current application state
	// ...

	//!!!DBG TMP!
	if (Exiting) FAIL;

	OK;
}
//---------------------------------------------------------------------

//!!!Init()'s pair!
void CApplication::Term()
{
}
//---------------------------------------------------------------------

void CApplication::ExitOnWindowClosed(Sys::COSWindow* pWindow)
{
	if (pWindow)
	{
		DISP_SUBSCRIBE_PEVENT(pWindow, OnClosing, CApplication, OnMainWindowClosing);
	}
	else
	{
		UNSUBSCRIBE_EVENT(OnClosing);
	}
}
//---------------------------------------------------------------------

bool CApplication::OnMainWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	UNSUBSCRIBE_EVENT(OnClosing);

	//FSM.RequestState(CStrID::Empty);

	//!!!DBG TMP!
	Exiting = true;

	OK;
}
//---------------------------------------------------------------------

}};
