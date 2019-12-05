#pragma once
#ifndef __DEM_L1_FRAME_RENDER_PHASE_H__
#define __DEM_L1_FRAME_RENDER_PHASE_H__

#include <Core/Object.h>
#include <Data/FixedArray.h>
#include <Data/StringID.h>
#include <Data/Flags.h>

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

	virtual ~CRenderPhase() {}

	virtual bool Init(const CRenderPath& Owner, CStrID PhaseName, const Data::CParams& Desc) { Name = PhaseName; OK; }
	virtual bool Render(CView& View) = 0;
};

typedef Ptr<CRenderPhase> PRenderPhase;

}

#endif
