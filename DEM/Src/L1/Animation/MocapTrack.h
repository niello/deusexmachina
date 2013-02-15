#pragma once
#ifndef __DEM_L1_ANIM_MOCAP_TRACK_H__
#define __DEM_L1_ANIM_MOCAP_TRACK_H__

#include <Animation/Anim.h>
#include <mathlib/vector.h>

// Mocap track is almost the same as animation track, but stores a bit less data, since some
// values are shared among all tracks of the same mocap clip.
//!!!maybe merge - it could be not worth splitting classes to save some bytes!

namespace Anim
{

class CMocapTrack
{
public:

	//???ptr to owner clip?
	vector4		ConstValue;
	//vector4*	pKeys;
	int			FirstKey;
	EChannel	Channel;

	//???WrapIfBefore, WrapIfAfter? or in clip?

	//???send relative time here? between 0 and 1
	void Sample(float Time, vector3& Out) {}
	void Sample(float Time, vector4& Out) {}
	void Sample(float Time, quaternion& Out) {}
};


}

#endif
