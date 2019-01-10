#include "D3D9SwapChain.h"

#include <Render/DisplayDriver.h>
#include <Render/RenderTarget.h>
#include <System/OSWindow.h>
#include <Events/Subscription.h>

#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
CD3D9SwapChain::CD3D9SwapChain() {}

CD3D9SwapChain::~CD3D9SwapChain()
{
	if (IsValid()) Destroy();
}
//---------------------------------------------------------------------

void CD3D9SwapChain::Release()
{
	Sub_OnClosing = nullptr;
	Sub_OnSizeChanged = nullptr;
	Sub_OnToggleFullscreen = nullptr;

	if (BackBufferRT.IsValidPtr()) BackBufferRT->Destroy();

	SAFE_RELEASE(pSwapChain);
}
//---------------------------------------------------------------------

void CD3D9SwapChain::Destroy()
{
	Release();
	TargetDisplay = nullptr;
	TargetWindow = nullptr;
	BackBufferRT = nullptr;
}
//---------------------------------------------------------------------

}
