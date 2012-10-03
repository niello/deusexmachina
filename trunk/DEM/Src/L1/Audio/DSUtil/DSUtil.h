//-----------------------------------------------------------------------------
// File: DSUtil.h
//
// Desc:
//
// Copyright (c) 1999-2001 Microsoft Corp. All rights reserved.
// Modified by Niello, 2012
//-----------------------------------------------------------------------------
#ifndef DSUTIL_H
#define DSUTIL_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

namespace Audio
{
	class CAudioFile;
}

class CDSSound
{
protected:

	Audio::CAudioFile*		m_pWaveFile;
	LPDIRECTSOUNDBUFFER*	m_apDSBuffer;
	DWORD					m_dwDSBufferSize;
	DWORD					m_dwNumBuffers;
	DWORD					m_dwCreationFlags;

	HRESULT RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL* pbWasRestored);

public:

	CDSSound(LPDIRECTSOUNDBUFFER* apDSBuffer, DWORD dwDSBufferSize, DWORD dwNumBuffers, Audio::CAudioFile* pWaveFile, DWORD dwCreationFlags);
	virtual ~CDSSound();

	virtual HRESULT Get3DBufferInterface(DWORD dwIndex, LPDIRECTSOUND3DBUFFER* ppDS3DBuffer);
	virtual HRESULT FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB, BOOL bRepeatWavIfBufferLarger);
	virtual LPDIRECTSOUNDBUFFER GetFreeBuffer(DWORD& index);
	virtual LPDIRECTSOUNDBUFFER GetBuffer(DWORD dwIndex);

	virtual HRESULT Play(DWORD dwPriority, DWORD dwFlags, LONG lVolume, LONG lFrequency, LONG lPan, DWORD& outIndex);
	virtual HRESULT Play3D(LPDS3DBUFFER p3DBuffer, DWORD dwPriority, DWORD dwFlags, LONG lVolume, LONG lFrequency, DWORD& outIndex);
	virtual HRESULT Stop();                     // stops all sound buffers
	virtual HRESULT Stop(DWORD index);          // stops only indexed sound buffer
	virtual HRESULT Reset();                    // resets all sound buffers
	virtual HRESULT Reset(DWORD index);         // resets only indexed sound buffer
	virtual BOOL	IsSoundPlaying();              // returns true if any sound buffer is playing
	virtual BOOL	IsSoundPlaying(DWORD index);   // returns true if a specific sound buffer is playing
};
//=====================================================================

// Desc: Encapsulates functionality to play a wave file with DirectSound.
//       The Create() method loads a chunk of wave file into the buffer,
//       and as sound plays more is written to the buffer by calling
//       HandleWaveStreamNotification() whenever hNotifyEvent is signaled.
class CDSStreamingSound: public CDSSound
{
protected:

	DWORD	m_dwLastPlayPos;
	DWORD	m_dwPlayProgress;
	DWORD	m_dwNotifySize;
	DWORD	m_dwTriggerWriteOffset;
	DWORD	m_dwNextWriteOffset;
	BOOL	m_bFillNextNotificationWithSilence;
	DWORD	stopCount;

public:

	CDSStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer, DWORD dwDSBufferSize, Audio::CAudioFile* pWaveFile, DWORD dwNotifySize, DWORD dwCreationFlags);
	virtual ~CDSStreamingSound();

	virtual HRESULT HandleWaveStreamNotification(BOOL bLoopedPlay);
	virtual HRESULT Reset();
	virtual BOOL IsSoundPlaying();              // returns true if any sound buffer is playing

	/// FLOH extended: return true when HandleWaveStreamNotification() must be called
	bool CheckStreamUpdate();
	bool Inside(DWORD pos, DWORD start, DWORD end);
};
//=====================================================================

#endif // DSUTIL_H
