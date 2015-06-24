#include "D3D11SwapChain.h"

#include <Render/RenderTarget.h>

#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{

void CD3D11SwapChain::Release()
{
	Sub_OnClosing = NULL;
	Sub_OnSizeChanged = NULL;
	Sub_OnToggleFullscreen = NULL;

	BackBufferRT->Destroy();

	//???
	TargetDisplay = NULL;
	TargetWindow = NULL;

	if (pSwapChain)
	{
		if (IsFullscreen()) pSwapChain->SetFullscreenState(FALSE, NULL);
		pSwapChain->Release();
		pSwapChain = NULL;
	}
}
//---------------------------------------------------------------------

}
