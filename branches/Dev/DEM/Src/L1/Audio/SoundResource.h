#pragma once
#ifndef __DEM_L1_SOUND_RESOURCE_H__
#define __DEM_L1_SOUND_RESOURCE_H__

//------------------------------------------------------------------------------
/**
    @class CSoundResource
    @ingroup Audio3

    A sound resource is a container for sound data which can be played back
    by the nAudioServer3 subsystem. The sound may be static or streaming,
    oneshot or looping. A sound resource should be able play itself
    several times simultaneously, the intended number of parallel
    "tracks" can be set by the user before opening the resource.

    Sound resources are generally shared and are referenced by
    CDSSound objects (there should be one CDSSound object per "sound instance",
    but several CDSSound objects should reference the same CSoundResource object,
    if the CDSSound objects sound the same).

    (C) 2003 RadonLabs GmbH
*/

#include <StdDEM.h>

class CDSSound;

namespace Audio
{

//???split to static & streaming here?
class CSoundResource
{
protected:

	CDSSound*	pDSSound;

	virtual bool LoadResource();
	virtual void UnloadResource();

public:

	int		NumTracks;
	bool	Ambient;
	bool	Streaming;
	bool	Looping;

	CSoundResource(): NumTracks(5), Ambient(false), Streaming(false), Looping(false), pDSSound(NULL) {}
	virtual ~CSoundResource() {} //{ if (IsLoaded()) Unload(); }

	CDSSound*	GetCSoundPtr() { return pDSSound; }
	//HANDLE		GetNotifyEvent();
	void*		GetNotifyEvent();
};

}

#endif

