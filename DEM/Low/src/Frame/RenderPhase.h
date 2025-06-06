#pragma once
#include <Core/Object.h>
#include <Data/StringID.h>

// Frame rendering phase implements a single algorithm in a render path.
// Algorithm may implement rendering into RT/MRT/DSV, advanced culling
// technique or whatever user wants.

namespace Data
{
	class CParams;
}

namespace Frame
{
class CGraphicsResourceManager;
class CView;
class CRenderPath;

class CRenderPhase: public DEM::Core::CObject //???need? lives only in RP!
{
	RTTI_CLASS_DECL(Frame::CRenderPhase, DEM::Core::CObject);

protected:

	CStrID Name;

public:

	virtual bool Init(CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc) { Name = PhaseName; OK; }
	virtual bool Render(CView& View) = 0;
};

typedef Ptr<CRenderPhase> PRenderPhase;

}
