#include "CompositePoseClip.h"

namespace DEM::Anim
{

void CCompositePoseClip::BindToOutput(const PPoseOutput& Output)
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