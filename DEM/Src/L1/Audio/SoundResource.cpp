#include "SoundResource.h"

#include <Audio/DSUtil/DSUtil.h>
#include <Audio/AudioServer.h>
#include <Audio/WAVFile.h>
#include <Audio/OGGFile.h>

namespace Audio
{

bool CSoundResource::LoadResource()
{
	/*
	n_assert(!IsLoaded() && !pDSSound);

	LPDIRECTSOUND8 pDS = AudioSrv->GetDirectSound();
	n_assert(pDS);

	CString FileName = GetFilename();
	FileName.ToLower();

	Audio::CAudioFile* pWaveFile;
	if (FileName.CheckExtension("wav")) pWaveFile = n_new(Audio::CWAVFile);
	else if (FileName.CheckExtension("ogg")) pWaveFile = n_new(Audio::COGGFile);
	else Core::Error("Audio file format not supported: %s\n", FileName.CStr());
	n_assert(pWaveFile);
	pWaveFile->Open(FileName.CStr());

	DSBUFFERDESC DSBufDesc;
	ZeroMemory(&DSBufDesc, sizeof(DSBUFFERDESC));
	DSBufDesc.dwSize = sizeof(DSBUFFERDESC);
	DSBufDesc.guid3DAlgorithm = DS3DALG_DEFAULT;
	DSBufDesc.lpwfxFormat = pWaveFile->GetFormat();
	DSBufDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_LOCDEFER;
	if (!Ambient) DSBufDesc.dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE;
	// for 3D sound: n_assert(pWaveFile->WaveFmt->nChannels == 1); ?

	if (Streaming) // create a streaming sound object (with a 128 KByte streaming buffer)
	{
		DSBufDesc.dwFlags |= DSBCAPS_GETCURRENTPOSITION2;
		//hr = pSoundMgr->CreateStreaming(&pDSSound, .CStr(), CreationFlags, DS3DALG_DEFAULT, 4, (1 << 15));

		DSBufDesc.dwBufferBytes = (1 << 17);

		LPDIRECTSOUNDBUFFER pDSBuffer = NULL;
		if (FAILED(pDS->CreateSoundBuffer(&DSBufDesc, &pDSBuffer, NULL)))
			Core::Error("CSoundResource::LoadResource(): CreateSoundBuffer for '%s' failed!", FileName.CStr());

		pDSSound = n_new(CDSStreamingSound(pDSBuffer, DSBufDesc.dwBufferBytes, pWaveFile, (1 << 15), DSBufDesc.dwFlags));
	}
	else
	{
		n_assert(NumTracks > 0);

		LPDIRECTSOUNDBUFFER* pDSBuffers = n_new_array(LPDIRECTSOUNDBUFFER, NumTracks);
		n_assert(pDSBuffers);

		DSBufDesc.dwBufferBytes = n_max(pWaveFile->GetSize() , DSBSIZE_FX_MIN);

		// DirectSound is only guarenteed to play PCM data. Other
		// formats may or may not work depending the sound card driver.
		// Return value can be DS_NO_VIRTUALIZATION!
		if (FAILED(pDS->CreateSoundBuffer(&DSBufDesc, &pDSBuffers[0], NULL)))
		{
			// a) 3D sound allows only mono
			// b) DSERR_BUFFERTOOSMALL, if BufSize < DSBSIZE_FX_MIN && DSBCAPS_CTRLFX flag is set
			// c) Hardware buffer mixing was requested on a device that doesn't support it
			Core::Error("CSoundResource::LoadResource(): CreateSoundBuffer for '%s' failed!"
				"Make sure you do not try to play a stereo sound as 3D sound!", FileName.CStr());
			return false;
		}

		// Default to use DuplicateSoundBuffer() when created extra buffers since always
		// create a buffer that uses the same memory however DuplicateSoundBuffer() will fail if
		// DSBCAPS_CTRLFX is used, so use CreateSoundBuffer() instead in this case.
		if (DSBufDesc.dwFlags & DSBCAPS_CTRLFX)
		{
			for (int i = 1; i < NumTracks; ++i)
				n_assert(SUCCEEDED(pDS->CreateSoundBuffer(&DSBufDesc, &pDSBuffers[i], NULL)));
		}
		else
		{
			for (int i = 1; i < NumTracks; ++i)
				n_assert(SUCCEEDED(pDS->DuplicateSoundBuffer(pDSBuffers[0], &pDSBuffers[i])));
		}

		pDSSound = n_new(CDSSound(pDSBuffers, DSBufDesc.dwBufferBytes, NumTracks, pWaveFile, DSBufDesc.dwFlags));

		n_delete(pWaveFile);
		n_delete_array(pDSBuffers); //!!!can allocate right in CDSSound!
	}

	n_assert(pDSSound);

	SetState(Valid);
	*/
	return true;
}
//---------------------------------------------------------------------

void CSoundResource::UnloadResource()
{
//	n_assert(IsLoaded());
	if (pDSSound)
	{
		n_delete(pDSSound);
		pDSSound = NULL;
	}
//	SetState(Unloaded);
}
//---------------------------------------------------------------------

}
