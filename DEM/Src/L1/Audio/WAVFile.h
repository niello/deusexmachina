#pragma once
#ifndef __DEM_L1_WAV_FILE_H__
#define __DEM_L1_WAV_FILE_H__

#include "AudioFile.h"

// WAV reader
// NOTE: this is basically a cleaned-up rippoff of DSound-Framework's CWaveFile class.

namespace Audio
{

class CWAVFile: public CAudioFile
{
private:

	static bool mmioProcInstalled;

	WAVEFORMATEX*	m_pwfx;			// Pointer to WAVEFORMATEX structure
	HMMIO			m_hmmio;		// MM I/O handle for the WAVE
	MMCKINFO		m_ck;			// Multimedia RIFF chunk
	MMCKINFO		m_ckRiff;		// Use in opening a WAVE file
	DWORD			Size;			// The size of the wave file
	MMIOINFO		m_mmioinfoOut;
	ULONG			m_ulDataSize;

	bool ReadMMIO();
	void InstallMMIOProc();
	void UninstallMMIOProc();

public:

	CWAVFile();
	virtual ~CWAVFile() { if (_IsOpen) Close(); }

	virtual bool			Open(const nString& FileName);
	virtual void			Close();

	virtual uint			Read(void* pBuffer, uint BytesToRead);
	virtual bool			Reset();
	virtual int				GetSize() const { n_assert(_IsOpen); return Size; }
	virtual WAVEFORMATEX*	GetFormat() { return m_pwfx; }
};

}

#endif

