#pragma once
#ifndef __DEM_L1_ANIM_CTLR_PRIORITY_BLEND_H__
#define __DEM_L1_ANIM_CTLR_PRIORITY_BLEND_H__

#include <Scene/AnimController.h>

// Priority-blend controller blends inputs from a set of another controllers from the most
// to the least priority according to input weights until weight sum is 1.0f

namespace Anim
{

class CAnimControllerPriorityBlend: public Scene::CAnimController
{
protected:

	// Set of (ctlr + weight + priority) sorted by priority

public:

	// bool AddSource(ctlr, priority, weight), RemoveSource(ctlr), Clear()
	// don't accept src if local/world mismatches

	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CAnimControllerPriorityBlend> PAnimControllerPriorityBlend;

}

#endif
