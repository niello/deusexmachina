#pragma once
#ifndef __DEM_L1_ANIM_TRACK_H__
#define __DEM_L1_ANIM_TRACK_H__

#include <mathlib/vector.h>

// Animation track is a set of float4 keys, forming a curve. Being sampled in a certain time,
// it returns single interpolated float4 value, that can be used as any transformation component
// or as any other value that changes in time.

namespace Anim
{

class CAnimTrack
{
protected:

	vector4		ConstValue;
	vector4*	pKeys;
	//!!!need key times! can store ptr to centralized buffer, as with keys!
	DWORD		KeyCount;		//!!! For CMocapTrack - not here but in CMocapClip, and has KeyStride!
	EChannel	Channel;

	//???WrapIfBefore, WrapIfAfter?

public:

	CAnimTrack() {}

	vector4	Sample(float Time);
};

typedef Ptr<CMesh> PMesh;

}

#endif
