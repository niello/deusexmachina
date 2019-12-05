#pragma once
#ifndef __DEM_L1_ANIM_MOCAP_CLIP_H__
#define __DEM_L1_ANIM_MOCAP_CLIP_H__

#include <Animation/AnimClip.h>
#include <Animation/MocapTrack.h>
#include <Data/Dictionary.h>

// Mocap clip is a type of animation clips with the same count of keys in each
// curve. Keys are placed in even intervals. This enables specific optimizations.

namespace Anim
{

class CMocapClip: public CAnimClip
{
	FACTORY_CLASS_DECL;

protected:

	vector4*			pKeys;
	CArray<CMocapTrack>	Tracks;		//???use fixed array?

	float				InvKeyTime;
	UPTR				KeysPerCurve;
	UPTR				KeyStride;

	//???WrapIfBefore, WrapIfAfter? Loop?

public:

	CMocapClip(): pKeys(nullptr) {}
	virtual ~CMocapClip() { Unload(); }

	bool							Setup(	const CArray<CMocapTrack>& _Tracks, const CArray<CStrID>& TrackMapping,
											const CArray<CEventTrack>* _EventTracks, vector4* _pKeys,
											UPTR _KeysPerCurve, UPTR _KeyStride, float _KeyTime);
	virtual void					Unload();

	virtual bool					IsResourceValid() const { return !!pKeys; }

	virtual Scene::PNodeController	CreateController(UPTR SamplerIdx) const;

	void							GetSamplingParams(float CursorPos, bool Loop, IPTR& KeyIndex, float& IpolFactor) const;
	const vector4&					GetKey(IPTR FirstKey, IPTR Index) const;
};

typedef Ptr<CMocapClip> PMocapClip;

inline const vector4& CMocapClip::GetKey(IPTR FirstKey, IPTR Index) const
{
	IPTR Idx = FirstKey + Index * KeyStride;
	n_assert_dbg(pKeys && Idx >= 0 && Idx < (int)KeysPerCurve * (int)KeyStride);
	return pKeys[Idx];
}
//---------------------------------------------------------------------

inline void CMocapClip::GetSamplingParams(float CursorPos, bool Loop, IPTR& OutKeyIndex, float& OutIpolFactor) const
{
	float KeyIdx = AdjustCursorPos(CursorPos, Loop) * InvKeyTime;
	OutKeyIndex = (int)KeyIdx;
	OutIpolFactor = KeyIdx - (float)OutKeyIndex;

	n_assert_dbg(OutKeyIndex >= 0 && OutKeyIndex < (IPTR)KeysPerCurve);
	n_assert_dbg(OutIpolFactor >= 0.f && OutIpolFactor < 1.f);
	n_assert_dbg(OutIpolFactor == 0.f || OutKeyIndex < (IPTR)(KeysPerCurve - 1)); // Never interpolate next the last key
}
//---------------------------------------------------------------------

}

#endif
