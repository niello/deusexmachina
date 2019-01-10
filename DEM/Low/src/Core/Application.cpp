#include "Application.h"

#include <Events/EventServer.h>
#include <Events/Subscription.h>
#include <Math/Math.h>
#include <System/Platform.h>
#include <System/Win32/OSWindowWin32.h>

namespace DEM { namespace Core
{

CApplication::CApplication(Sys::IPlatform& _Platform)
	: Platform(_Platform)
{
	//???move RNG instance to an application? pass system time inside?
	Math::InitRandomNumberGenerator();

	// create default file system from platform
	// setup hard assigns from platform and application
}
//---------------------------------------------------------------------

CApplication::~CApplication()
{
}
//---------------------------------------------------------------------

Sys::POSWindow CApplication::CreateRenderWindow()
{
	//!!!DBG TMP! fix title & icon!
	//???subscribe on destroying / closing?
	//???pass size here to create in a final size?
	auto Wnd = Platform.CreateGUIWindow();
	//Windows.Add(Wnd);
	return Wnd;
}
//---------------------------------------------------------------------

bool CApplication::Run()
{
	//???here or in constructor?
	n_new(Events::CEventServer);

	BaseTime = Platform.GetSystemTime();
	PrevTime = BaseTime;
	FrameTime = 0.0;

	// initialize systems

	// start FSM initial state
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

void CApplication::Close()
{
	//???here or in destructor?
	if (Events::CEventServer::HasInstance()) n_delete(EventSrv);
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
