#pragma once
#ifndef __DEM_L1_ANIM_KEYFRAME_TRACK_H__
#define __DEM_L1_ANIM_KEYFRAME_TRACK_H__

#include <Animation/AnimFwd.h>
#include <Math/Quaternion.h>
#include <Data/FixedArray.h>

// Animation track is a set of float4 keys, forming a curve. Being sampled at a certain time,
// it returns single interpolated float4 value, that can be used as any transformation component
// or as any other value that changes in time.
// Keyframe track stores key data inside. It offers more flexibility and less memory consumption
// in a cost of some calculations.

//???templated track for different types?
//!!!compressed track! for vectors/quaternions (and maybe other floats) only!

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

	CFixedArray<CKey> Keys;

	void Sample(float Time, vector3& Out);
	void Sample(float Time, vector4& Out);
	void Sample(float Time, quaternion& Out);
};

}

#endif
