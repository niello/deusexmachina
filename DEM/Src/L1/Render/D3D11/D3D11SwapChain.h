#pragma once
#ifndef __DEM_L1_RENDER_D3D11_SWAP_CHAIN_H__
#define __DEM_L1_RENDER_D3D11_SWAP_CHAIN_H__

#include <Render/SwapChain.h>
#include <Events/EventsFwd.h>

// Direct3D11 implementation of a swap chain

struct IDXGISwapChain;

namespace Render
{

//???store index and driver pointer and subscribe events here?
class CD3D11SwapChain: public CSwapChain
{
public:

	//???need? DXGI handles OS window!
	Events::PSub		Sub_OnToggleFullscreen;
	Events::PSub		Sub_OnSizeChanged;
	Events::PSub		Sub_OnClosing;

	IDXGISwapChain*		pSwapChain;
	//ID3D11Texture2D*	pBackBuffer;
	//!!!RT View! or unify RT of swap chain and texture?

	CD3D11SwapChain(): pSwapChain(NULL) {}
	~CD3D11SwapChain() { Release(); }

	void Release();
};

}

#endif
