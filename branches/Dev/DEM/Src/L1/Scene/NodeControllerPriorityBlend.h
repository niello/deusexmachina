#pragma once
#ifndef __DEM_L1_ANIM_CTLR_PRIORITY_BLEND_H__
#define __DEM_L1_ANIM_CTLR_PRIORITY_BLEND_H__

#include <Scene/NodeControllerComposite.h>

// Priority-blend controller blends inputs from a set of another controllers from the most
// to the least priority according to input weights until weight sum is 1.0f

//???what is the best way to blend multiple quaternions?
//!!!there is QuatSquad blending operation!

namespace Scene
{

class CNodeControllerPriorityBlend: public CNodeControllerComposite
{
public:

	virtual bool ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CNodeControllerPriorityBlend> PNodeControllerPriorityBlend;

}

#endif
