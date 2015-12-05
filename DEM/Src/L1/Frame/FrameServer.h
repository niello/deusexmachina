#pragma once
#ifndef __DEM_L1_FRAME_SERVER_H__
#define __DEM_L1_FRAME_SERVER_H__

#include <Data/Singleton.h>
#include <Data/HandleManager.h>

// Frame server controls frame rendering. Frame subsystem communicates with Scene and Render
// subsystems, rendering scene views to viewports or intermediate render targets. View is
// defined by a scene (what to render), a camera (from where), render target(s) (to where)
// and a render path (how). NULL scene is valid and has meaning for example for GUI-only views.

namespace Scene
{
// SPS or root node
};

namespace Render
{
	class CRenderTarget;
};

namespace Frame
{
typedef HHandle HView;
class CCamera;

#define FrameSrv Frame::CFrameServer::Instance()

class CFrameServer
{
	__DeclareSingleton(CFrameServer);

private:

	// render path collection
	// view collection - for iteration

	Data::CHandleManager	HandleMgr;

public:

	CFrameServer() { __ConstructSingleton; }
	~CFrameServer() { __DestructSingleton; }

	//???RPs as resources?
	// LoadRenderPath() / GetRenderPath() //???ever need user access to RP object outside the frame subsys?

	HView	RegisterView(/*SPS or root scene node, camera, RP, RT array ptr & count, opt depth-stencil buffer*/);
	void	UnregisterView(HView hView);
	bool	RenderView(HView hView);
	bool	RenderAllViews();
};

}

#endif
