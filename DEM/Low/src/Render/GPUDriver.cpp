#include "GPUDriver.h"
#include <Render/GPUFence.h>
#include <System/OSWindow.h>

namespace Render
{

bool CGPUDriver::PresentBlankScreen(UPTR SwapChainID, const vector4& ColorRGBA)
{
	if (!BeginFrame()) FAIL;
	Clear(Clear_Color, ColorRGBA, 1.f, 0);
	EndFrame();
	return Present(SwapChainID);
}
//---------------------------------------------------------------------

void CGPUDriver::PrepareWindowAndBackBufferSize(DEM::Sys::COSWindow& Window, U32& Width, U32& Height)
{
	// Zero Width or Height means backbuffer matching window or display size.
	// But if at least one of these values specified, we should adjst window size.
	// A child window is an exception, we don't want renderer to resize it,
	// so we force a backbuffer size to a child window size.
	if (Width > 0 || Height > 0)
	{
		if (Window.IsChild())
		{
			Width = Window.GetWidth();
			Height = Window.GetHeight();
		}
		else
		{
			Data::CRect WindowRect = Window.GetRect();
			if (Width > 0) WindowRect.W = Width;
			else Width = WindowRect.W;
			if (Height > 0) WindowRect.H = Height;
			else Height = WindowRect.H;
			Window.SetRect(WindowRect);
		}
	}
}
//---------------------------------------------------------------------

PGPUFence CGPUDriver::SignalFence()
{
	PGPUFence Fence = CreateFence();
	if (!SignalFence(*Fence)) Fence = nullptr;
	return Fence;
}
//---------------------------------------------------------------------

}
