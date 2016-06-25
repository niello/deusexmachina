#pragma once
#ifndef __DEM_L1_UI_RENDERER_H__
#define __DEM_L1_UI_RENDERER_H__

#include <Frame/RenderPhase.h>

// Frame rendering phase 

namespace Frame
{

class CRenderPhaseGUI: public CRenderPhase
{
	__DeclareClass(CRenderPhaseGUI);

private:

	I32 RenderTargetIndex;

public:

	virtual bool Init(CStrID PhaseName, const Data::CParams& Desc);
	virtual bool Render(CView& View);
};

typedef Ptr<CRenderPhaseGUI> PRenderPhaseGUI;

}

#endif
