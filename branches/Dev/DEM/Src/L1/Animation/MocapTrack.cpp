#include "MocapTrack.h"

#include <Animation/MocapClip.h>

namespace Anim
{

void CMocapTrack::Sample(int KeyIndex, float IpolFactor, vector3& Out)
{
	n_assert_dbg(pOwnerClip && IpolFactor >= 0.f);

	if (FirstKey == INVALID_INDEX)
	{
		Out.set(ConstValue.x, ConstValue.y, ConstValue.z);
		return;
	}

	const vector4& Key = pOwnerClip->GetKey(FirstKey, KeyIndex);

	if (IpolFactor > 0.f)
	{
		const vector4& NextKey = pOwnerClip->GetKey(FirstKey, KeyIndex + 1);
		Out.lerp((const vector3&)Key, (const vector3&)NextKey, IpolFactor);
	}
	else Out.set(Key.x, Key.y, Key.z);
}
//---------------------------------------------------------------------

void CMocapTrack::Sample(int KeyIndex, float IpolFactor, vector4& Out)
{
	n_assert_dbg(pOwnerClip && IpolFactor >= 0.f);

	if (FirstKey == INVALID_INDEX)
	{
		Out.set(ConstValue.x, ConstValue.y, ConstValue.z, ConstValue.w);
		return;
	}

	const vector4& Key = pOwnerClip->GetKey(FirstKey, KeyIndex);

	if (IpolFactor > 0.f)
	{
		const vector4& NextKey = pOwnerClip->GetKey(FirstKey, KeyIndex + 1);
		Out.lerp(Key, NextKey, IpolFactor);
	}
	else Out.set(Key.x, Key.y, Key.z, Key.w);
}
//---------------------------------------------------------------------

void CMocapTrack::Sample(int KeyIndex, float IpolFactor, quaternion& Out)
{
	n_assert_dbg(pOwnerClip && IpolFactor >= 0.f);

	if (FirstKey == INVALID_INDEX)
	{
		Out.set(ConstValue.x, ConstValue.y, ConstValue.z, ConstValue.w);
		return;
	}

	const vector4& Key = pOwnerClip->GetKey(FirstKey, KeyIndex);

	if (IpolFactor > 0.f)
	{
		const vector4& NextKey = pOwnerClip->GetKey(FirstKey, KeyIndex + 1);
		Out.slerp((const quaternion&)Key, (const quaternion&)NextKey, IpolFactor);
	}
	else Out.set(Key.x, Key.y, Key.z, Key.w);
}
//---------------------------------------------------------------------

}