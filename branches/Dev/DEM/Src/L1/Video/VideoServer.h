#pragma once
#ifndef __DEM_L1_VIDEO_SERVER_H__
#define __DEM_L1_VIDEO_SERVER_H__

#include <Core/Object.h>
#include <Data/Singleton.h>
#include <Data/Array.h>
#include <dshow.h>

// Server object to playback video streams.
// DirectShow implementation.

namespace Video
{
class CVideoPlayer;

#define VideoSrv Video::CVideoServer::Instance()

class CVideoServer: public Core::CObject
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CVideoServer);

protected:

	CArray<CVideoPlayer*>	Players;

	bool					_IsOpen;
	bool					_IsPlaying;

	IGraphBuilder*			pGraphBuilder;
	IMediaControl*			pMediaCtl;
	IMediaEvent*			pMediaEvent;
	IVideoWindow*			pVideoWnd;
	IBasicVideo*			pBasicVideo;

public:

	bool					ScalingEnabled;

	CVideoServer();
	virtual ~CVideoServer();

	virtual bool	Open();
	virtual void	Close();
	virtual void	Trigger();

	virtual bool	PlayFile(const char* pFileName);
	virtual void	Stop();

	CVideoPlayer*	NewVideoPlayer(const char* pName);
	void			DeleteVideoPlayer(CVideoPlayer* pPlayer);

	bool			IsOpen() const { return _IsOpen; }
	bool			IsPlaying() const { return _IsPlaying; }
};

}

#endif

