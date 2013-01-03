//-----------------------------------------------------------------------------
// File: DSUtil.cpp
//
// Desc: DirectSound framework classes for reading and writing wav files and
//       playing them in DirectSound buffers. Feel free to use this class
//       as a starting point for adding extra functionality.
//
// Copyright (c) Microsoft Corp. All rights reserved.
// Modified by Niello, 2012
//-----------------------------------------------------------------------------
#define STRICT
#include "DSUtil.h"
#include <StdDEM.h>
#include <Audio/WAVFile.h>
#include <Audio/OGGFile.h>
#define WIN32_LEAN_AND_MEAN
#include <mmreg.h>
#include <msacm.h>
#include <dxerr.h>

//-----------------------------------------------------------------------------
// Name: CDSSound::CDSSound()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
CDSSound::CDSSound(LPDIRECTSOUNDBUFFER* apDSBuffer, DWORD DSBufferSize,
			   DWORD dwNumBuffers, Audio::CAudioFile* pWaveFile, DWORD dwCreationFlags)
{
	m_apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
	n_assert(m_apDSBuffer);
	for (DWORD i = 0; i < dwNumBuffers; ++i)
		m_apDSBuffer[i] = apDSBuffer[i];

	m_dwDSBufferSize = DSBufferSize;
	m_dwNumBuffers = dwNumBuffers;
	m_pWaveFile = pWaveFile;
	m_dwCreationFlags = dwCreationFlags;

	FillBufferWithSound(m_apDSBuffer[0], FALSE);
}

//-----------------------------------------------------------------------------
// Name: CDSSound::~CDSSound()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
CDSSound::~CDSSound()
{
	for (DWORD i = 0; i < m_dwNumBuffers; ++i)
		SAFE_RELEASE(m_apDSBuffer[i]);
	SAFE_DELETE_ARRAY(m_apDSBuffer);
}


//-----------------------------------------------------------------------------
// Name: CDSSound::FillBufferWithSound()
// Desc: Fills a DirectSound buffer with a sound file
//-----------------------------------------------------------------------------
HRESULT CDSSound::FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB, BOOL bRepeatWavIfBufferLarger)
{
    HRESULT hr;
    VOID* pDSLockedBuffer = NULL;
    DWORD dwDSLockedBufferSize = 0;

    if (!m_pWaveFile || !pDSB) return CO_E_NOTINITIALIZED;

    // Make sure we have focus, and we didn't just switch in from
    // an app which had a DirectSound device
    if (FAILED(hr = RestoreBuffer(pDSB, NULL)))
        return DXTRACE_ERR(TEXT("RestoreBuffer"), hr);

    if (FAILED(hr = pDSB->Lock(0, m_dwDSBufferSize, &pDSLockedBuffer, &dwDSLockedBufferSize, NULL, NULL, 0L)))
        return DXTRACE_ERR(TEXT("Lock"), hr);

    int hr1 = S_OK;
    int hr2 = S_OK;

    m_pWaveFile->Reset();
    DWORD dwWavDataRead = m_pWaveFile->Read(pDSLockedBuffer, dwDSLockedBufferSize);
	WORD BitsPerSample = m_pWaveFile->GetFormat()->wBitsPerSample;

    if (dwWavDataRead == 0)
    {
        // Wav is blank, so just fill with silence
        FillMemory((BYTE*)pDSLockedBuffer, dwDSLockedBufferSize, (BYTE)(BitsPerSample == 8 ? 128 : 0));
    }
    else if (dwWavDataRead < dwDSLockedBufferSize)
    {
        // If the wav file was smaller than the DirectSound buffer,
        // we need to fill the remainder of the buffer with data
        if (bRepeatWavIfBufferLarger)
        {
            // Reset the file and fill the buffer with wav data
            DWORD dwReadSoFar = dwWavDataRead;    // From previous call above.
            while (dwReadSoFar < dwDSLockedBufferSize)
            {
                // This will keep reading in until the buffer is full
                // for very short files
                m_pWaveFile->Reset();
                dwWavDataRead = m_pWaveFile->Read((char*)pDSLockedBuffer + dwReadSoFar,
                                                  dwDSLockedBufferSize - dwReadSoFar);

                dwReadSoFar += dwWavDataRead;
            }
        }
        else
        {
            // Don't repeat the file, just fill in silence
            FillMemory((BYTE*) pDSLockedBuffer + dwWavDataRead,
                        dwDSLockedBufferSize - dwWavDataRead,
                        (BYTE)(BitsPerSample == 8 ? 128 : 0));
        }
    }

    // Unlock the buffer, we don't need it anymore.
    pDSB->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CDSSound::RestoreBuffer()
// Desc: Restores the lost buffer. *pbWasRestored returns TRUE if the buffer was
//       restored.  It can also NULL if the information is not needed.
//-----------------------------------------------------------------------------
HRESULT CDSSound::RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL* pbWasRestored)
{
    HRESULT hr;

    if (!pDSB) return CO_E_NOTINITIALIZED;
    if (pbWasRestored) *pbWasRestored = FALSE;

    DWORD dwStatus;
    if (FAILED(hr = pDSB->GetStatus(&dwStatus)))
        return DXTRACE_ERR(TEXT("GetStatus"), hr);

    if (dwStatus & DSBSTATUS_BUFFERLOST)
    {
        // Since the app could have just been activated, then
        // DirectSound may not be giving us control yet, so
        // the restoring the buffer may fail.
        // If it does, sleep until DirectSound gives us control.
        do
        {
            if (pDSB->Restore() == DSERR_BUFFERLOST) Sleep(10);
        }
        while ((hr = pDSB->Restore()) == DSERR_BUFFERLOST);

        if (pbWasRestored) *pbWasRestored = TRUE;

        return S_OK;
    }
    else return S_FALSE;
}


//-----------------------------------------------------------------------------
// Name: CDSSound::GetFreeBuffer()
// Desc: Finding the first buffer that is not playing and return a pointer to
//       it, or if all are playing return a pointer to a randomly selected buffer.
//
//  - 17-May-04     floh    additionally returns index of buffer
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER CDSSound::GetFreeBuffer(DWORD& index)
{
    index = 0;
    if (!m_apDSBuffer) return FALSE;

    for (; index < m_dwNumBuffers; index++)
        if (m_apDSBuffer[index])
        {
            DWORD dwStatus = 0;
            m_apDSBuffer[index]->GetStatus(&dwStatus);
            if (!(dwStatus & DSBSTATUS_PLAYING)) break;
        }

    if (index != m_dwNumBuffers) return m_apDSBuffer[index];
    else
    {
        index = rand() % m_dwNumBuffers;
        return m_apDSBuffer[index];
    }
}


//-----------------------------------------------------------------------------
// Name: CDSSound::GetBuffer()
// Desc:
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER CDSSound::GetBuffer(DWORD dwIndex)
{
	return (m_apDSBuffer && dwIndex < m_dwNumBuffers) ? m_apDSBuffer[dwIndex] : NULL;
}


//-----------------------------------------------------------------------------
// Name: CDSSound::Get3DBufferInterface()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDSSound::Get3DBufferInterface(DWORD dwIndex, LPDIRECTSOUND3DBUFFER* ppDS3DBuffer)
{
	if (!m_apDSBuffer) return CO_E_NOTINITIALIZED;
	if (dwIndex >= m_dwNumBuffers) return E_INVALIDARG;
	*ppDS3DBuffer = NULL;
	return m_apDSBuffer[dwIndex]->QueryInterface(IID_IDirectSound3DBuffer, (VOID**)ppDS3DBuffer);
}


//-----------------------------------------------------------------------------
// Name: CDSSound::Play()
// Desc: Plays the sound using voice management flags.  Pass in DSBPLAY_LOOPING
//       in the dwFlags to loop the sound
//-----------------------------------------------------------------------------
HRESULT CDSSound::Play(DWORD dwPriority, DWORD dwFlags, LONG lVolume, LONG lFrequency, LONG lPan, DWORD& outIndex)
{
    HRESULT hr;
    BOOL bRestored;

    if (m_apDSBuffer == NULL)
        return CO_E_NOTINITIALIZED;

    LPDIRECTSOUNDBUFFER pDSB = GetFreeBuffer(outIndex);

    if (pDSB == NULL)
        return DXTRACE_ERR(TEXT("GetFreeBuffer"), E_FAIL);

    // Restore the buffer if it was lost
    if (FAILED(hr = RestoreBuffer(pDSB, &bRestored)))
        return DXTRACE_ERR(TEXT("RestoreBuffer"), hr);

    if (bRestored)
    {
        // The buffer was restored, so we need to fill it with new data
        if (FAILED(hr = FillBufferWithSound(pDSB, FALSE)))
            return DXTRACE_ERR(TEXT("FillBufferWithSound"), hr);
    }

    if (m_dwCreationFlags & DSBCAPS_CTRLVOLUME) pDSB->SetVolume(lVolume);

    if (lFrequency != -1 && (m_dwCreationFlags & DSBCAPS_CTRLFREQUENCY))
        pDSB->SetFrequency(lFrequency);

    if (m_dwCreationFlags & DSBCAPS_CTRLPAN) pDSB->SetPan(lPan);

    pDSB->SetCurrentPosition(0);
    return pDSB->Play(0, dwPriority, dwFlags);
}




//-----------------------------------------------------------------------------
// Name: CDSSound::Play3D()
// Desc: Plays the sound using voice management flags.  Pass in DSBPLAY_LOOPING
//       in the dwFlags to loop the sound
//-----------------------------------------------------------------------------
HRESULT CDSSound::Play3D(LPDS3DBUFFER p3DBuffer, DWORD dwPriority, DWORD dwFlags, LONG lVolume, LONG lFrequency, DWORD& outIndex)
{
    HRESULT hr;
    BOOL    bRestored;
    DWORD   dwBaseFrequency;

    if (!m_apDSBuffer) return CO_E_NOTINITIALIZED;

    LPDIRECTSOUNDBUFFER pDSB = GetFreeBuffer(outIndex);
    if (!pDSB) return DXTRACE_ERR(TEXT("GetFreeBuffer"), E_FAIL);

    // Restore the buffer if it was lost
    if (FAILED(hr = RestoreBuffer(pDSB, &bRestored)))
        return DXTRACE_ERR(TEXT("RestoreBuffer"), hr);

    if (bRestored)
        if (FAILED(hr = FillBufferWithSound(pDSB, FALSE)))
            return DXTRACE_ERR(TEXT("FillBufferWithSound"), hr);

    if (m_dwCreationFlags & DSBCAPS_CTRLFREQUENCY)
    {
        pDSB->GetFrequency(&dwBaseFrequency);
        pDSB->SetFrequency(dwBaseFrequency + lFrequency);
    }
    if (m_dwCreationFlags & DSBCAPS_CTRLVOLUME) pDSB->SetVolume(lVolume);

    // QI for the 3D buffer
    LPDIRECTSOUND3DBUFFER pDS3DBuffer;
    hr = pDSB->QueryInterface(IID_IDirectSound3DBuffer, (VOID**) &pDS3DBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pDS3DBuffer->SetAllParameters(p3DBuffer, DS3D_IMMEDIATE);
        if (SUCCEEDED(hr))
        {
            hr = pDSB->SetCurrentPosition(0);
            hr = pDSB->Play(0, dwPriority, dwFlags);
        }
        pDS3DBuffer->Release();
    }

    return hr;
}



//-----------------------------------------------------------------------------
// Name: CDSSound::Stop()
// Desc: Stops all sound from playing
//-----------------------------------------------------------------------------
HRESULT CDSSound::Stop()
{
	if (!m_apDSBuffer) return CO_E_NOTINITIALIZED;

	HRESULT hr = 0;
	for (DWORD i = 0; i < m_dwNumBuffers; ++i)
		hr |= m_apDSBuffer[i]->Stop();

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CDSSound::Stop(index)
// Desc: Stops only the indexed sound from playing
//-----------------------------------------------------------------------------
HRESULT CDSSound::Stop(DWORD index)
{
	if (!m_apDSBuffer) return CO_E_NOTINITIALIZED;
	if (index < 0 || index >= m_dwNumBuffers) return DSERR_INVALIDPARAM;
	return m_apDSBuffer[index]->Stop();
}

//-----------------------------------------------------------------------------
// Name: CDSSound::Reset()
// Desc: Reset all of the sound buffers
//-----------------------------------------------------------------------------
HRESULT CDSSound::Reset()
{
	if (!m_apDSBuffer) return CO_E_NOTINITIALIZED;

	HRESULT hr = 0;
	for (DWORD i = 0; i < m_dwNumBuffers; ++i)
		hr |= m_apDSBuffer[i]->SetCurrentPosition(0);

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CDSSound::Reset(index)
// Desc: Reset only the indexed sound buffer
//-----------------------------------------------------------------------------
HRESULT CDSSound::Reset(DWORD index)
{
	if (!m_apDSBuffer) return CO_E_NOTINITIALIZED;
	if (index < 0 || index >= m_dwNumBuffers) return DSERR_INVALIDPARAM;
	return m_apDSBuffer[index]->SetCurrentPosition(0);
}

//-----------------------------------------------------------------------------
// Name: CDSSound::IsSoundPlaying()
// Desc: Checks to see if any buffer is playing and returns TRUE if it is.
//-----------------------------------------------------------------------------
BOOL CDSSound::IsSoundPlaying()
{
	if (m_apDSBuffer)
		for (DWORD i = 0; i < m_dwNumBuffers; ++i)
			if (m_apDSBuffer[i])
			{
				DWORD dwStatus = 0;
				m_apDSBuffer[i]->GetStatus(&dwStatus);
				if (dwStatus & DSBSTATUS_PLAYING) return TRUE;
			}
	return FALSE;
}

//-----------------------------------------------------------------------------
// Name: CDSSound::IsSoundPlaying(soundIndex)
// Desc: Checks to see if a specific buffer is playing and returns TRUE if it is.
//-----------------------------------------------------------------------------
BOOL CDSSound::IsSoundPlaying(DWORD Idx)
{
	if (m_apDSBuffer && Idx >= 0 && Idx < m_dwNumBuffers && m_apDSBuffer[Idx])
	{
		DWORD dwStatus = 0;
		m_apDSBuffer[Idx]->GetStatus(&dwStatus);
		if (dwStatus & DSBSTATUS_PLAYING) return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSoundCStreamingSound()
// Desc: Setups up a buffer so data can be streamed from the wave file into
//       a buffer.  This is very useful for large wav files that would take a
//       while to load.  The buffer is initially filled with data, then
//       as sound is played the notification events are signaled and more data
//       is written into the buffer by calling HandleWaveStreamNotification()
//-----------------------------------------------------------------------------
CDSStreamingSound::CDSStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer, DWORD DSBufferSize,
                                 Audio::CAudioFile* pWaveFile, DWORD dwNotifySize, DWORD dwCreationFlags)
                : CDSSound(&pDSBuffer, DSBufferSize, 1, pWaveFile, dwCreationFlags)
{
	m_dwLastPlayPos = 0;
	m_dwPlayProgress = 0;
	m_dwNotifySize = dwNotifySize;
	m_dwTriggerWriteOffset = 0;
	m_dwNextWriteOffset = 0;
	m_bFillNextNotificationWithSilence = FALSE;
	stopCount = 0;
}

//-----------------------------------------------------------------------------
// Name: CDSStreamingSound::~CDSStreamingSound()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
CDSStreamingSound::~CDSStreamingSound()
{
	SAFE_DELETE(m_pWaveFile);
}

//-----------------------------------------------------------------------------
// Name: CDSStreamingSound::IsSoundPlaying()
// Desc: Streaming sounds need different behavior
//-----------------------------------------------------------------------------
BOOL CDSStreamingSound::IsSoundPlaying()
{
	return CDSSound::IsSoundPlaying() && stopCount <= 1;
}

//-----------------------------------------------------------------------------
// Name: CDSStreamingSound::HandleWaveStreamNotification()
// Desc: Handle the notification that tells us to put more wav data in the
//       circular buffer
//-----------------------------------------------------------------------------
HRESULT CDSStreamingSound::HandleWaveStreamNotification(BOOL bLoopedPlay)
{
	if (!m_apDSBuffer || !m_pWaveFile) return CO_E_NOTINITIALIZED;

	HRESULT hr;
	DWORD   dwCurrentPlayPos;
	DWORD   dwPlayDelta;
	DWORD   dwBytesWrittenToBuffer;
	VOID*   pDSLockedBuffer = NULL;
	VOID*   pDSLockedBuffer2 = NULL;
	DWORD   dwDSLockedBufferSize;
	DWORD   dwDSLockedBufferSize2;

    // Restore the buffer if it was lost
    BOOL bRestored;
    if (FAILED(hr = RestoreBuffer(m_apDSBuffer[0], &bRestored)))
        return DXTRACE_ERR(TEXT("RestoreBuffer"), hr);

    if (bRestored)
    {
        // The buffer was restored, so we need to fill it with new data
        if (FAILED(hr = FillBufferWithSound(m_apDSBuffer[0], FALSE)))
            return DXTRACE_ERR(TEXT("FillBufferWithSound"), hr);
        return S_OK;
    }

    // Lock the DirectSound buffer
    if (FAILED(hr = m_apDSBuffer[0]->Lock(m_dwNextWriteOffset, m_dwNotifySize,
                                          &pDSLockedBuffer, &dwDSLockedBufferSize,
                                          &pDSLockedBuffer2, &dwDSLockedBufferSize2, 0L)))
        return DXTRACE_ERR(TEXT("Lock"), hr);

    // m_dwDSBufferSize and m_dwNextWriteOffset are both multiples of m_dwNotifySize,
    // it should the second buffer, so it should never be valid
    if (pDSLockedBuffer2) return E_UNEXPECTED;

	WORD BitsPerSample = m_pWaveFile->GetFormat()->wBitsPerSample;
    if (!m_bFillNextNotificationWithSilence)
    {
        dwBytesWrittenToBuffer = m_pWaveFile->Read((char*) pDSLockedBuffer, dwDSLockedBufferSize);
    }
    else
    {
        // Fill the DirectSound buffer with silence
        this->stopCount++;
        FillMemory(pDSLockedBuffer, dwDSLockedBufferSize,
                   (BYTE)(BitsPerSample == 8 ? 128 : 0));
        dwBytesWrittenToBuffer = dwDSLockedBufferSize;
    }

    // If the number of bytes written is less than the
    // amount we requested, we have a short file.
    if (dwBytesWrittenToBuffer < dwDSLockedBufferSize)
    {
        if (!bLoopedPlay)
        {
            // Fill in silence for the rest of the buffer.
            FillMemory((BYTE*) pDSLockedBuffer + dwBytesWrittenToBuffer,
                       dwDSLockedBufferSize - dwBytesWrittenToBuffer,
                       (BYTE)(BitsPerSample == 8 ? 128 : 0));

            // Any future notifications should just fill the buffer with silence
            m_bFillNextNotificationWithSilence = TRUE;
        }
        else
        {
            // We are looping, so reset the file and fill the buffer with wav data
            DWORD dwReadSoFar = dwBytesWrittenToBuffer;    // From previous call above.
            while (dwReadSoFar < dwDSLockedBufferSize)
            {
                // This will keep reading in until the buffer is full (for very short files).
                m_pWaveFile->Reset();
                dwBytesWrittenToBuffer = m_pWaveFile->Read((char*)pDSLockedBuffer + dwReadSoFar,
                                                           dwDSLockedBufferSize - dwReadSoFar);
                n_assert(dwBytesWrittenToBuffer > 0);
                dwReadSoFar += dwBytesWrittenToBuffer;
            }
        }
    }

    // Unlock the DirectSound buffer
    m_apDSBuffer[0]->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0);

    // Figure out how much data has been played so far.  When we have played
    // past the end of the file, we will either need to start filling the
    // buffer with silence or starting reading from the beginning of the file,
    // depending if the user wants to loop the sound
    if (FAILED(hr = m_apDSBuffer[0]->GetCurrentPosition(&dwCurrentPlayPos, NULL)))
        return DXTRACE_ERR(TEXT("GetCurrentPosition"), hr);

    // Check to see if the position counter looped
    if (dwCurrentPlayPos < m_dwLastPlayPos)
        dwPlayDelta = (m_dwDSBufferSize - m_dwLastPlayPos) + dwCurrentPlayPos;
    else
        dwPlayDelta = dwCurrentPlayPos - m_dwLastPlayPos;

    m_dwPlayProgress += dwPlayDelta;
    m_dwLastPlayPos = dwCurrentPlayPos;

    // If we are now filling the buffer with silence, then we have found the end so
    // check to see if the entire sound has played, if it has then stop the buffer.
    if (m_bFillNextNotificationWithSilence)
    {
        // We don't want to cut off the sound before it's done playing.
        if (m_dwPlayProgress >= (DWORD)m_pWaveFile->GetSize())
            m_apDSBuffer[0]->Stop();
    }

    // Update where the buffer will lock (for next time)
    m_dwNextWriteOffset += dwDSLockedBufferSize;
    m_dwTriggerWriteOffset = m_dwNextWriteOffset - m_dwNotifySize;
    m_dwNextWriteOffset %= m_dwDSBufferSize; // Circular buffer
    m_dwTriggerWriteOffset %= m_dwDSBufferSize;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CDSStreamingSound::Inside()
// Desc: Returns true when the provided buffer offset is within the
//       start and end position (with wraparound)
//-----------------------------------------------------------------------------
bool CDSStreamingSound::Inside(DWORD pos, DWORD start, DWORD end)
{
	if (start < end) return pos >= start && pos < end;
	else return pos >= start || pos < end;
}

//-----------------------------------------------------------------------------
// Name: CDSStreamingSound::CheckStreamUpdate()
// Desc: Returns true when the HandleWaveStreamNotification() method
//       must be called.
//-----------------------------------------------------------------------------
bool CDSStreamingSound::CheckStreamUpdate()
{
	DWORD PlayCursor, WriteCursor;
	m_apDSBuffer[0]->GetCurrentPosition(&PlayCursor, &WriteCursor);
	return Inside(WriteCursor, m_dwTriggerWriteOffset, m_dwNextWriteOffset);
}

//-----------------------------------------------------------------------------
// Name: CDSStreamingSound::Reset()
// Desc: Resets the sound so it will begin playing at the beginning
//-----------------------------------------------------------------------------
HRESULT CDSStreamingSound::Reset()
{
	HRESULT hr;

	if (!m_apDSBuffer[0] || !m_pWaveFile) return CO_E_NOTINITIALIZED;

	m_dwLastPlayPos     = 0;
	m_dwPlayProgress    = 0;
	m_dwNextWriteOffset = 0;
	m_dwTriggerWriteOffset = m_dwDSBufferSize - m_dwNotifySize;
	m_bFillNextNotificationWithSilence = FALSE;
	stopCount = 0;

	// Restore the buffer if it was lost
	BOOL bRestored;
	if (FAILED(hr = RestoreBuffer(m_apDSBuffer[0], &bRestored)))
		return DXTRACE_ERR(TEXT("RestoreBuffer"), hr);

	if (bRestored)
		if (FAILED(hr = FillBufferWithSound(m_apDSBuffer[0], FALSE)))
			return DXTRACE_ERR(TEXT("FillBufferWithSound"), hr);

	m_pWaveFile->Reset();
	return m_apDSBuffer[0]->SetCurrentPosition(0L);
}
