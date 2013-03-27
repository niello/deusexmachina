#pragma once
#ifndef __DEM_L1_ANIM_MOCAP_TRACK_H__
#define __DEM_L1_ANIM_MOCAP_TRACK_H__

#include <Animation/Anim.h>
#include <StdDEM.h>
#include <mathlib/quaternion.h>

// Mocap track is almost the same as animation track, but stores a bit less data, since some
// values are shared among all tracks of the same mocap clip.

namespace Anim
{
class CMocapClip;

class CMocapTrack
{
public:

	CMocapClip*	pOwnerClip;
	vector4		ConstValue;
	int			FirstKey;
	EChannel	Channel;

	CMocapTrack(): pOwnerClip(NULL), FirstKey(INVALID_INDEX) {}

	void Sample(int KeyIndex, float IpolFactor, vector3& Out);
	void Sample(int KeyIndex, float IpolFactor, vector4& Out);
	void Sample(int KeyIndex, float IpolFactor, quaternion& Out);
};

}

#endif
