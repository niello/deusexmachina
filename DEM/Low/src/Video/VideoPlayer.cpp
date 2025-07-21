#include "VideoPlayer.h"

namespace Video
{

CVideoPlayer::CVideoPlayer():
	_IsOpen(false),
	FileName(""),
	loopType(Repeat)
{
}
//---------------------------------------------------------------------

bool CVideoPlayer::Open()
{
	n_assert(!_IsOpen);
	_IsOpen = true;
	return true;
}
//---------------------------------------------------------------------

void CVideoPlayer::Close()
{
	n_assert(_IsOpen);
	_IsOpen = false;
}
//---------------------------------------------------------------------

}
