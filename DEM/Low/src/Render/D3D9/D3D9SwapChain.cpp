#include "D3D9SwapChain.h"

#include <Render/DisplayDriver.h>
#include <Render/RenderTarget.h>
#include <System/OSWindow.h>
#include "DEMD3D9.h"

namespace Render
{
CD3D9SwapChain::CD3D9SwapChain() = default;

CD3D9SwapChain::~CD3D9SwapChain()
{
	if (IsValid()) Destroy();
}
//---------------------------------------------------------------------

void CD3D9SwapChain::Release()
{
	Sub_OnClosing.Disconnect();
	Sub_OnToggleFullscreen.Disconnect();

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
