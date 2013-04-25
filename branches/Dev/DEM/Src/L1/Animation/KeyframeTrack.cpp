#include "KeyframeTrack.h"

namespace Anim
{

void CKeyframeTrack::Sample(float Time, vector3& Out)
{
	n_assert(Time > 0.f); // && Time <= Clip->Duration

	if (!Keys.Size())
	{
		Out.set(ConstValue.x, ConstValue.y, ConstValue.z);
		return;
	}

	//!!!perform binary search for the closest less or equal
	//???or search from last key in direction dependent on Time - LastTime sign?
	int i = 0;
	for (; i < Keys.Size(); ++i)
		if (Keys[i].Time > Time) break;
	--i;

	// Extend the first key back to the 0 and the last key forward to the clip duration
	if (i < 0 || i == Keys.Size() - 1 || Keys[i].Time == Time)
	{
		const CKey& Key = Keys[i];
		Out.set(Key.Value.x, Key.Value.y, Key.Value.z);
	}
	else
	{
		const CKey& Key = Keys[i];
		const CKey& NextKey = Keys[i + 1];
		float IpolFactor = (Time - Key.Time) / (NextKey.Time - Key.Time);
		Out.lerp((const vector3&)Key.Value, (const vector3&)NextKey.Value, IpolFactor);
	}
}
//---------------------------------------------------------------------

void CKeyframeTrack::Sample(float Time, vector4& Out)
{
	n_assert(Time > 0.f); // && Time <= Clip->Duration

	if (!Keys.Size())
	{
		Out.set(ConstValue.x, ConstValue.y, ConstValue.z, ConstValue.w);
		return;
	}

	//!!!perform binary search for the closest less or equal
	//???or search from last key in direction dependent on Time - LastTime sign?
	int i = 0;
	for (; i < Keys.Size(); ++i)
		if (Keys[i].Time > Time) break;
	--i;

	// Extend the first key back to the 0 and the last key forward to the clip duration
	if (i < 0 || i == Keys.Size() - 1 || Keys[i].Time == Time)
	{
		const CKey& Key = Keys[i];
		Out.set(Key.Value.x, Key.Value.y, Key.Value.z, Key.Value.w);
	}
	else
	{
		const CKey& Key = Keys[i];
		const CKey& NextKey = Keys[i + 1];
		float IpolFactor = (Time - Key.Time) / (NextKey.Time - Key.Time);
		Out.lerp(Key.Value, NextKey.Value, IpolFactor);
	}
}
//---------------------------------------------------------------------

void CKeyframeTrack::Sample(float Time, quaternion& Out)
{
	n_assert(Time > 0.f); // && Time <= Clip->Duration

	if (!Keys.Size())
	{
		Out.set(ConstValue.x, ConstValue.y, ConstValue.z, ConstValue.w);
		return;
	}

	//!!!perform binary search for the closest less or equal
	//???or search from last key in direction dependent on Time - LastTime sign?
	int i = 0;
	for (; i < Keys.Size(); ++i)
		if (Keys[i].Time > Time) break;
	--i;

	// Extend the first key back to the 0 and the last key forward to the clip duration
	if (i < 0 || i == Keys.Size() - 1 || Keys[i].Time == Time)
	{
		const CKey& Key = Keys[i];
		Out.set(Key.Value.x, Key.Value.y, Key.Value.z, Key.Value.w);
	}
	else
	{
		const CKey& Key = Keys[i];
		const CKey& NextKey = Keys[i + 1];
		float IpolFactor = (Time - Key.Time) / (NextKey.Time - Key.Time);
		Out.slerp((const quaternion&)Key.Value, (const quaternion&)NextKey.Value, IpolFactor);
	}
}
//---------------------------------------------------------------------

}