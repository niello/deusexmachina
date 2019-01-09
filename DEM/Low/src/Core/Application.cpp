#include "Application.h"

#include <System/Platform.h>
#include <System/Win32/OSWindowWin32.h>

namespace DEM { namespace Core
{

CApplication::CApplication(Sys::IPlatform& _Platform)
	: Platform(_Platform)
{
}
//---------------------------------------------------------------------

Sys::POSWindow CApplication::CreateRenderWindow()
{
	//!!!DBG TMP! fix title & icon!
	//???subscribe on destroying / closing?
	//???pass size here to create in a final size?
	auto Wnd = Platform.CreateGUIWindow("TestWnd", nullptr);
	//Windows.Add(Wnd);
	return Wnd;
}
//---------------------------------------------------------------------

bool CApplication::Run()
{
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

	OK;
}
//---------------------------------------------------------------------

void CApplication::Close()
{
}
//---------------------------------------------------------------------

}};
