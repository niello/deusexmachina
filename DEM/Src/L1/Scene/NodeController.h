#pragma once
#ifndef __DEM_L1_SCENE_NODE_CTLR_H__
#define __DEM_L1_SCENE_NODE_CTLR_H__

#include <Core/RefCounted.h>
#include <Animation/Anim.h>
#include <Data/Flags.h>
#include <Math/TransformSRT.h>

// Scene node controller provides transform (SRT) parameters. It is mainly used for
// scene node animation. Animation controller subclasses can sample keyframed
// animation data, use physics as a transformation source or implement any other
// custom logic, such as look-at or even constant value. Animation controller can
// also implement selection or blending algorithm using other animation controllers
// as inputs.

//!!!there is QuatSquad blending operation!

namespace Scene
{

class CNodeController: public Core::CRefCounted
{
protected:

	enum
	{
		Active				= 0x01,
		LocalSpace			= 0x02,
		UpdateLocalSpace	= 0x04
	};

	Data::CFlags	Channels;
	Data::CFlags	Flags;

public:

	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm) = 0;

	void			Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
	bool			IsActive() const { return Flags.Is(Active); }
	bool			IsLocalSpace() const { return Flags.Is(LocalSpace); }
	bool			NeedToUpdateLocalSpace() const { return Flags.Is(UpdateLocalSpace); }
	bool			HasChannel(Anim::EChannel Channel) const { return Channels.Is(Channel); }
};

typedef Ptr<CNodeController> PNodeController;

}

#endif
