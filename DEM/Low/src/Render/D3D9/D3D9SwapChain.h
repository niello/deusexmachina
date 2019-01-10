#pragma once
#ifndef __DEM_L1_RENDER_D3D9_SWAP_CHAIN_H__
#define __DEM_L1_RENDER_D3D9_SWAP_CHAIN_H__

#include <Render/SwapChain.h>
#include <Events/EventsFwd.h>

// Direct3D9 implementation of a swap chain

struct IDirect3DSwapChain9;

namespace Render
{

class CD3D9SwapChain: public CSwapChain
{
public:

	virtual ~CD3D9SwapChain();

	IDirect3DSwapChain9*	pSwapChain = nullptr; // NULL for implicit swap chain, device methods will be called

	Events::PSub			Sub_OnToggleFullscreen;
	Events::PSub			Sub_OnSizeChanged;
	Events::PSub			Sub_OnClosing;

	void Release();
	void Destroy();
};

}

#endif
