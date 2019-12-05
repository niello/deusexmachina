#pragma once
#include <Frame/RenderPhase.h>
#include <UI/UIFwd.h>

// Frame rendering phase 

namespace Frame
{

class CRenderPhaseGUI: public CRenderPhase
{
	FACTORY_CLASS_DECL;

private:

	CStrID			RenderTargetID;
	UI::EDrawMode	DrawMode;

public:

	virtual bool Init(const CRenderPath& Owner, CStrID PhaseName, const Data::CParams& Desc);
	virtual bool Render(CView& View);
};

typedef Ptr<CRenderPhaseGUI> PRenderPhaseGUI;

}
