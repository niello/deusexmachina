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
class CView;
class CRenderPath;

class CRenderPhase: public Core::CObject //???need? lives only in RP!
{
	RTTI_CLASS_DECL;

protected:

	CStrID Name;

public:

	virtual bool Init(const CRenderPath& Owner, CStrID PhaseName, const Data::CParams& Desc) { Name = PhaseName; OK; }
	virtual bool Render(CView& View) = 0;
};

typedef Ptr<CRenderPhase> PRenderPhase;

}
