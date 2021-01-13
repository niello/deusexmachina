#include "CompositePoseClip.h"

namespace DEM::Anim
{

PPoseClipBase CCompositePoseClip::Clone() const
{
	NOT_IMPLEMENTED;
	return nullptr;
}
//---------------------------------------------------------------------

void CCompositePoseClip::GatherSkeletonInfo(PSkeletonInfo& SkeletonInfo)
{
	// bind each track to blender, bind blender to output
}
//---------------------------------------------------------------------

void CCompositePoseClip::PlayInterval(float PrevTime, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR ClipIndex)
{
	// evaluate blend params
	// evaluate tracks
	// perform blending into the output
}
//---------------------------------------------------------------------

}
