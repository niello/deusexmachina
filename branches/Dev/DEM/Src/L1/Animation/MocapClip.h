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
	__DeclareClass(CMocapClip);

protected:

	vector4*			pKeys;
	CArray<CMocapTrack>	Tracks;		//???use fixed array?

	float				InvKeyTime;
	DWORD				KeysPerCurve;
	DWORD				KeyStride;

	//???WrapIfBefore, WrapIfAfter? Loop?

public:

	CMocapClip(CStrID ID): CAnimClip(ID), pKeys(NULL) {}
	virtual ~CMocapClip() { if (IsLoaded()) Unload(); }

	bool							Setup(	const CArray<CMocapTrack>& _Tracks, const CArray<CStrID>& TrackMapping,
											const CArray<CEventTrack>* _EventTracks, vector4* _pKeys,
											DWORD _KeysPerCurve, DWORD _KeyStride, float _KeyTime);
	virtual void					Unload();

	virtual Scene::PNodeController	CreateController(DWORD SamplerIdx) const;

	void							GetSamplingParams(float Time, bool Loop, int& KeyIndex, float& IpolFactor) const;
	const vector4&					GetKey(int FirstKey, int Index) const;
};

typedef Ptr<CMocapClip> PMocapClip;

inline const vector4& CMocapClip::GetKey(int FirstKey, int Index) const
{
	int Idx = FirstKey + Index * KeyStride;
	n_assert_dbg(pKeys && Idx >= 0 && Idx < (int)KeysPerCurve * (int)KeyStride);
	return pKeys[Idx];
}
//---------------------------------------------------------------------

inline void CMocapClip::GetSamplingParams(float Time, bool Loop, int& OutKeyIndex, float& OutIpolFactor) const
{
	float KeyIdx = AdjustTime(Time, Loop) * InvKeyTime;
	OutKeyIndex = (int)KeyIdx;
	OutIpolFactor = KeyIdx - (float)OutKeyIndex;

	n_assert_dbg(OutKeyIndex >= 0 && OutKeyIndex < (int)KeysPerCurve);
	n_assert_dbg(OutIpolFactor >= 0.f && OutIpolFactor < 1.f);
	n_assert_dbg(OutIpolFactor == 0.f || OutKeyIndex < (int)(KeysPerCurve - 1)); // Never interpolate next the last key
}
//---------------------------------------------------------------------

}

#endif
