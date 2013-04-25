#pragma once
#ifndef __DEM_L1_ANIM_KEYFRAME_TRACK_H__
#define __DEM_L1_ANIM_KEYFRAME_TRACK_H__

#include <Animation/Anim.h>
#include <mathlib/quaternion.h>
#include <util/nfixedarray.h>

// Animation track is a set of float4 keys, forming a curve. Being sampled in a certain time,
// it returns single interpolated float4 value, that can be used as any transformation component
// or as any other value that changes in time.

namespace Anim
{

class CKeyframeTrack: public CAnimTrack
{
public:

	struct CKey
	{
		vector4	Value;
		float	Time;
	};

	nFixedArray<CKey>	Keys;
	vector4				ConstValue;
	EChannel			Channel; //!!!???can avoid storing it? needed only at Setup() time, move to loader?

	CKeyframeTrack() {}

	void Sample(float Time, vector3& Out);
	void Sample(float Time, vector4& Out);
	void Sample(float Time, quaternion& Out);
};

}

#endif
