#pragma once
#ifndef __DEM_L1_NODE_CTLR_STATIC_H__
#define __DEM_L1_NODE_CTLR_STATIC_H__

#include <Scene/NodeController.h>

// Controller, that locks node in a static pose. Useful for blending with fading-in
// animations or for transitions from arbitrary pose to animation starting pose.

namespace Scene
{

class CNodeControllerStatic: public CNodeController
{
protected:

	Math::CTransformSRT StaticTfm;

public:

	CNodeControllerStatic() { Channels.Set(Tfm_Scaling | Tfm_Rotation | Tfm_Translation); }

	void			SetStaticTransform(const Math::CTransformSRT Tfm, bool Local) { StaticTfm = Tfm; Flags.SetTo(LocalSpace, Local); }
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm) { DestTfm = StaticTfm; OK; }
};

typedef Ptr<CNodeControllerStatic> PNodeControllerStatic;

}

#endif
