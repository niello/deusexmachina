#pragma once
#ifndef __DEM_L1_ANIM_H__
#define __DEM_L1_ANIM_H__

#include <StdDEM.h>

// Animation system constants and forward declarations

template<class TKey, class TVal> class nDictionary;

namespace Anim
{

//!!!temporarily ID is a bone index! (to make Kila move)
typedef int CBoneID;

enum EChannel
{
	Chnl_Translation	= 0x01,
	Chnl_Rotation		= 0x02,
	Chnl_Scaling		= 0x04
};

/*
enum ELoopType
{
	LoopType_Clamp,
	LoopType_Loop
};
*/

class CAnimTrack {};

struct CSampler
{
	CAnimTrack* pTrackT;
	CAnimTrack* pTrackR;
	CAnimTrack* pTrackS;

	CSampler(): pTrackT(NULL), pTrackR(NULL), pTrackS(NULL) {}
};

typedef nDictionary<CBoneID, CSampler> CSamplerList;

}

#endif