#pragma once
#ifndef __DEM_L1_FRAME_SERVER_H__
#define __DEM_L1_FRAME_SERVER_H__

#include <Data/Singleton.h>
#include <Data/HandleManager.h>

// Frame server controls frame rendering. Frame subsystem communicates with Scene and Render
// subsystems, rendering scene views to viewports or intermediate render targets.

namespace Frame
{
class CView;

#define FrameSrv Frame::CFrameServer::Instance()

class CFrameServer
{
	__DeclareSingleton(CFrameServer);

private:

	// render path collection
	// view collection - for iteration

	Data::CHandleManager	HandleMgr; //???templated and store views, not ptrs?

public:

	CFrameServer() { __ConstructSingleton; }
	~CFrameServer() { __DestructSingleton; }

	//???RPs as resources?
	// LoadRenderPath() / GetRenderPath() //???ever need user access to RP object outside the frame subsys?

	HHandle	RegisterView(CView* pView); //???who controls view (de)allocation?
	void	UnregisterView(HHandle hView);
	bool	RenderView(HHandle hView);
	bool	RenderAllViews();
};

}

#endif
