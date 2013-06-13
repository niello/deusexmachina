#pragma once
#ifndef __DEM_L1_ANIM_MOCAP_TRACK_H__
#define __DEM_L1_ANIM_MOCAP_TRACK_H__

#include <Animation/AnimFwd.h>
#include <mathlib/quaternion.h>

// Animation track is a set of float4 keys, forming a curve. Being sampled in a certain time,
// it returns single interpolated float4 value, that can be used as any transformation component
// or as any other value that changes in time.
// Mocap track references key data stored in a clip. Mocap keys use relatively more memory than
// regular keyframe data, but some calculations may be performed once per frame for all tracks.

namespace Anim
{
class CMocapClip;

class CMocapTrack: public CAnimTrack
{
public:

	CMocapClip*	pOwnerClip;
	int			FirstKey;

	CMocapTrack(): pOwnerClip(NULL), FirstKey(INVALID_INDEX) {}

	void Sample(int KeyIndex, float IpolFactor, vector3& Out);
	void Sample(int KeyIndex, float IpolFactor, vector4& Out);
	void Sample(int KeyIndex, float IpolFactor, quaternion& Out);
};

}

#endif
