#pragma once
#ifndef __DEM_L1_AUDIO_FILE_H__
#define __DEM_L1_AUDIO_FILE_H__

#include <Data/String.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

#ifndef __WIN32__
#error "Win32 specific class!"
#endif

// A generic base class for read access to audio files.
// Derive subclass for specific file formats/codecs.
// Win32-specific implementation (only WAVEFORMATEX), because it is required by DirectSound.

namespace Audio
{

class CAudioFile
{
protected:

	bool _IsOpen;

public:

	CAudioFile(): _IsOpen(false) {}
	virtual ~CAudioFile() { n_assert(!_IsOpen); }

	virtual bool			Open(const CString& FileName);
	virtual void			Close();
	bool					IsOpen() const { return _IsOpen; }

	virtual uint			Read(void* pBuffer, uint BytesToRead);
	virtual bool			Reset();
	virtual int				GetSize() const;
	virtual WAVEFORMATEX*	GetFormat() = 0;
};

}

#endif
