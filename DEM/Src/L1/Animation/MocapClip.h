#pragma once
#ifndef __DEM_L1_ANIM_MOCAP_CLIP_H__
#define __DEM_L1_ANIM_MOCAP_CLIP_H__

#include <Resources/Resource.h>
#include <Animation/MocapTrack.h>
#include <util/ndictionary.h>

// Mocap clip is a type of animation clips with the same count of keys in each
// curve. Keys are placed in even intervals. This enables specific optimizations.

namespace Anim
{

//!!!temporarily ID is a bone index! (to make Kila move)
typedef int CBoneID;

//!!!Must have shared parent with CAnimClip!
class CMocapClip: public Resources::CResource
{
public:

	struct CSampler
	{
		CMocapTrack* pTrackT;
		CMocapTrack* pTrackR;
		CMocapTrack* pTrackS;

		CSampler(): pTrackT(NULL), pTrackR(NULL), pTrackS(NULL) {}
	};

	typedef nDictionary<CBoneID, CSampler> CSamplerList;

protected:

	vector4*			pKeys;
	nArray<CMocapTrack>	Tracks;
	CSamplerList		Samplers;

	float				Duration;
	float				InvKeyTime;
	DWORD				KeysPerCurve;
	DWORD				KeyStride;

	//???WrapIfBefore, WrapIfAfter? Loop?

public:

	CMocapClip(CStrID ID, Resources::IResourceManager* pHost): CResource(ID, pHost), pKeys(NULL) {}

	bool			Setup(	const nArray<CMocapTrack>& _Tracks, const nArray<CBoneID>& TrackMapping, vector4* _pKeys,
							DWORD _KeysPerCurve, DWORD _KeyStride, float _KeyTime);
	virtual void	Unload();

	const CSamplerList&	GetSamplerList() const { return Samplers; }
	//const CSampler*		GetSampler(CBoneID NodeID) const { return Samplers.Get(NodeID); }
	void				GetSamplingParams(float Time, bool Loop, int& KeyIndex, float& IpolFactor) const;
	const vector4&		GetKey(int FirstKey, int Index) const;

	//!!!move to base animclip class!
	float			AdjustTime(float Time, bool Loop) const;
};

typedef Ptr<CMocapClip> PMocapClip;

inline const vector4& CMocapClip::GetKey(int FirstKey, int Index) const
{
	int Idx = FirstKey + Index * KeyStride;
	n_assert_dbg(pKeys && Idx >= 0 && Idx < (int)KeysPerCurve * (int)KeyStride);
	return pKeys[Idx];
}
//---------------------------------------------------------------------

inline float CMocapClip::AdjustTime(float Time, bool Loop) const
{
	if (Loop)
	{
		Time = n_fmod(Time, Duration);
		if (Time < 0.f) Time += Duration;
	}
	else
	{
		if (Time < 0.f) Time = 0.f;
		else if (Time > Duration) Time = Duration;
	}

	return Time;
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
