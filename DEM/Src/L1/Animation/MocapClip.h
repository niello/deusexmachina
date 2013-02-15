#pragma once
#ifndef __DEM_L1_ANIM_MOCAP_CLIP_H__
#define __DEM_L1_ANIM_MOCAP_CLIP_H__

#include <Resources/Resource.h>
#include <Animation/AnimControllerClip.h>
#include <util/ndictionary.h>

// Mocap clip is a type of animation clips with the same count of keys in each
// curve. Keys are placed in even intervals. This enables specific optimizations.

namespace Anim
{

//!!!Must have shared parent with CAnimClip!
class CMocapClip: public Resources::CResource
{
protected:

	//!!!temporarily mapped by bone index! (to make Kila move)
	nDictionary<int, PAnimControllerClip>	NodeToCtlr;
	float									Duration;
	DWORD									KeyCount;	//???need precalculated KeyDuration?
	DWORD									KeyStride;
	nArray<CMocapTrack>						Tracks;
	vector4*								pKeys;

public:

	CMocapClip(CStrID ID, Resources::IResourceManager* pHost): CResource(ID, pHost) {}

	bool					Setup(const nArray<CMocapTrack>& _Tracks, const nArray<int>& TrackMapping, vector4* _pKeys);
	virtual void			Unload();

	CAnimControllerClip*	GetController(int NodeID) const;
};

typedef Ptr<CMocapClip> PMocapClip;

inline CAnimControllerClip* CMocapClip::GetController(int NodeID) const
{
	int CtlrIdx = NodeToCtlr.FindIndex(NodeID);
	return (CtlrIdx == INVALID_INDEX) ? NULL : NodeToCtlr.ValueAtIndex(CtlrIdx).get_unsafe();
}
//---------------------------------------------------------------------

}

#endif
