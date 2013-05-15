#pragma once
#ifndef __IPG_APP_STATE_PLAY_VIDEO_H__
#define __IPG_APP_STATE_PLAY_VIDEO_H__

// Plays a video and waits for a esc or space key press, or that the video has finished.
// Based on mangalore PlayVideoHandler_(C) 2005 RadonLabs GmbH

#include <App/StateHandler.h>

namespace App
{

class CPlayVideoHandler: public CStateHandler
{
	__DeclareClassNoFactory;

private:

	CStrID	NextState;
	nString	VideoFileName;

public:

	bool	EnableScaling;

	CPlayVideoHandler(CStrID StateID): CStateHandler(StateID), EnableScaling(false) {}

	virtual void	OnStateEnter(CStrID PrevState, PParams Params = NULL);
	virtual CStrID	OnFrame();

	void			SetNextState(CStrID State) { NextState = State; }
	CStrID			GetNextState() const { return NextState; }
	void			SetVideoFile(const nString& FileName) { VideoFileName = FileName; }
	const nString&	GetVideoFile() const { return VideoFileName; }
};

}

#endif
