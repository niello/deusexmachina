#pragma once
#include <Render/SwapChain.h>
#include <Events/EventsFwd.h>

// Direct3D9 implementation of a swap chain

struct IDirect3DSwapChain9;

namespace Render
{

class CD3D9SwapChain: public CSwapChain
{
public:

	CD3D9SwapChain();
	virtual ~CD3D9SwapChain() override;

	IDirect3DSwapChain9*	pSwapChain = nullptr; // nullptr for implicit swap chain, device methods will be called

	Events::PSub			Sub_OnToggleFullscreen;
	Events::PSub			Sub_OnClosing;

	void Release();
	void Destroy();
};

}
