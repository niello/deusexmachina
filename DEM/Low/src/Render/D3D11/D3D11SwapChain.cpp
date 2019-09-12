#include "D3D11SwapChain.h"

#include <Render/DisplayDriver.h>
#include <Render/RenderTarget.h>
#include <System/OSWindow.h>
#include <Events/Subscription.h>

#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
CD3D11SwapChain::CD3D11SwapChain() {}

CD3D11SwapChain::~CD3D11SwapChain()
{
	if (IsValid()) Destroy();
}
//---------------------------------------------------------------------

void CD3D11SwapChain::Destroy()
{
	Sub_OnClosing = nullptr;
	Sub_OnSizeChanged = nullptr;
	Sub_OnToggleFullscreen = nullptr;

	if (BackBufferRT.IsValidPtr()) BackBufferRT->Destroy();

	if (pSwapChain)
	{
		if (IsFullscreen()) pSwapChain->SetFullscreenState(FALSE, nullptr);
		pSwapChain->Release();
		pSwapChain = nullptr;
	}

	TargetDisplay = nullptr;
	TargetWindow = nullptr;
	BackBufferRT = nullptr;
}
//---------------------------------------------------------------------

}
