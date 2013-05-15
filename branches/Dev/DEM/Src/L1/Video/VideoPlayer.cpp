#include "VideoPlayer.h"

#include <IO/IOServer.h>

namespace Video
{

CVideoPlayer::CVideoPlayer():
	_IsOpen(false),
	FileName(""),
	//pTexture(NULL),
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

void CVideoPlayer::SetFilename(const nString& _FileName)
{
	FileName = IOSrv->ManglePath(_FileName);
}
//---------------------------------------------------------------------

}