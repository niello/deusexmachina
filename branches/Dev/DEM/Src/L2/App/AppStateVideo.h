#pragma once
#ifndef __IPG_APP_STATE_PLAY_VIDEO_H__
#define __IPG_APP_STATE_PLAY_VIDEO_H__

// Plays a video and waits for a esc or space key press, or that the video has finished.
// Based on mangalore PlayVideoHandler_(C) 2005 RadonLabs GmbH

#include <App/StateHandler.h>

namespace App
{

class CAppStateVideo: public CStateHandler
{
	__DeclareClassNoFactory;

private:

	CStrID	NextState;
	CString	VideoFileName;

public:

	bool	EnableScaling;

	CAppStateVideo(CStrID StateID): CStateHandler(StateID), EnableScaling(false) {}

	virtual void	OnStateEnter(CStrID PrevState, Data::PParams Params = NULL);
	virtual CStrID	OnFrame();

	void			SetNextState(CStrID State) { NextState = State; }
	CStrID			GetNextState() const { return NextState; }
	void			SetVideoFile(const char* pFileName) { VideoFileName = pFileName; }
	const CString&	GetVideoFile() const { return VideoFileName; }
};

}

#endif
