#pragma once
#ifndef __DEM_L2_GFX_CHR_ANIM_EVT_HDL_H__
#define __DEM_L2_GFX_CHR_ANIM_EVT_HDL_H__

#include <anim2/nanimeventhandler.h>
#include <Gfx/Entity.h>

// An animation event handler for the Graphics::CCharEntity class.

namespace Graphics
{
typedef Ptr<class CEntity> PEntity;

class CCharAnimEHandler: public nAnimEventHandler
{
protected:

	nTime LastSoundTime;
	nTime LastVFXTime;

public:

	PEntity Entity;

	CCharAnimEHandler(): LastSoundTime(0.0), LastVFXTime(0.0) {}
	virtual ~CCharAnimEHandler() {}

	virtual void HandleEvent(const nAnimEventTrack& Track, int EventIdx);
};

typedef Ptr<CCharAnimEHandler> PCharAnimEHandler;

}

#endif
