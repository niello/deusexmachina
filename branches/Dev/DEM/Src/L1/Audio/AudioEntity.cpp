#include "AudioEntity.h"

#include "AudioServer.h"
#include <Time/TimeServer.h>

namespace Audio
{
ImplementRTTI(Audio::CAudioEntity, Core::CRefCounted);
ImplementFactory(Audio::CAudioEntity);

CAudioEntity::CAudioEntity():
	Volume(1.0f),
	FadeOutStartTime(0.0),
	FadeOutTime(0.0),
	Priority(0),
	minDist(10.0f),
	maxDist(100.0f),
	insideConeAngle(0),
	outsideConeAngle(360),
	coneOutsideVolume(1.0f),
	Category(Effect),
	Props3DDirty(true),
	SoundIdx(-1)
{
	memset(&DS3DProps, 0, sizeof(DS3DProps));
	DS3DProps.dwSize = sizeof(DS3DProps);
}
//---------------------------------------------------------------------

void CAudioEntity::Activate()
{
	n_assert(!Flags.Is(IS_ACTIVE));
	n_assert(RsrcName.IsValid());
	n_assert(!refRsrc.isvalid());

	//???!!!rename to GetSoundResource?!
	refRsrc = (CSoundResource*)AudioSrv->NewSoundResource(RsrcName.Get());
	n_assert(refRsrc.isvalid());

	if (!refRsrc->IsLoaded())
	{
		refRsrc->SetFilename(RsrcName);
		refRsrc->NumTracks = NumTracks;
		refRsrc->Ambient = Ambient;
		refRsrc->Streaming = Streaming;
		refRsrc->Looping = Looping;
		if (!refRsrc->Load())
		{
			refRsrc->Release();
			refRsrc.invalidate();
			return;
		}
	}

	Volume = 1.0f;
	Flags.Set(IS_ACTIVE);
}
//---------------------------------------------------------------------

void CAudioEntity::Deactivate()
{
	n_assert(Flags.Is(IS_ACTIVE));
	n_assert(refRsrc.isvalid());

	if (IsPlaying()) Stop();

	refRsrc->Release();
	refRsrc.invalidate();

	Flags.Clear(IS_ACTIVE);
}
//---------------------------------------------------------------------

void CAudioEntity::Start()
{
	//!!!only if has sound device!
	CDSSound* pSound = GetCSoundPtr();
	n_assert(pSound);

	if (Streaming)
	{
		// reset buffer and fill with new data
		CDSStreamingSound* pStreamSound = (CDSStreamingSound*)pSound;
		pStreamSound->Reset();
		LPDIRECTSOUNDBUFFER dsBuffer = pStreamSound->GetBuffer(0);
		pStreamSound->FillBufferWithSound(dsBuffer, Looping);
		SoundIdx = 0;
	}

	int PlaybackFlags = 0;
	if (Looping || Streaming) PlaybackFlags |= DSBPLAY_LOOPING;
	if (Ambient)
	{
		// Play as 2D sound
		if (FAILED(pSound->Play(Priority, PlaybackFlags, GetDSVolume(), 0, 0, SoundIdx)))
			n_printf("Sound: failed to start 2D sound '%s'\n", RsrcName.Get());
	}
	else
	{
		//!!!THE ONLY PLACE GetDS3DProps is used. Can split to 2D & 3D sounds instead of Ambient flag!
		// Play as 3D sound
		PlaybackFlags |= DSBPLAY_TERMINATEBY_PRIORITY | DSBPLAY_TERMINATEBY_DISTANCE;
		if (FAILED(pSound->Play3D(GetDS3DProps(), Priority, PlaybackFlags, GetDSVolume(), 0, SoundIdx)))
			n_printf("Sound: failed to start 3D sound '%s'\n", RsrcName.Get());
	}

	Flags.Set(IS_FIRST_FRAME);
}
//---------------------------------------------------------------------

// Stop sound playback (mostly only makes sense for looping sounds).
void CAudioEntity::Stop()
{
	//!!!only if has sound device!
	if (SoundIdx == -1) return;
	CDSSound* pSound = GetCSoundPtr();
	n_assert(pSound);
	n_assert(SoundIdx >= 0);
	pSound->Stop(SoundIdx);
	SoundIdx = -1;
}
//---------------------------------------------------------------------

void CAudioEntity::Update()
{
	if (IsPlaying())
	{
		if (Flags.Is(IS_FADING_OUT))
		{
			float Age = float(TimeSrv->GetTime() - FadeOutStartTime);
			if (Age < FadeOutTime) Volume = 1.0f - n_saturate(Age / float(FadeOutTime));
			else
			{
				Stop();
				Flags.Clear(IS_FADING_OUT);
			}
		}

		//!!!only if has sound device!
		//???!!!update only if this volume is dirty?
		//!!!!!!!check Srv->Mute!!!!!!
		if (SoundIdx != -1)
		{
			//??? N2: FIXME! CDSSound needs update method!
			CDSSound* pSound = GetCSoundPtr();
			n_assert(pSound);
			if (pSound->IsSoundPlaying())
				pSound->GetBuffer(Streaming ? 0 : SoundIdx)->SetVolume(GetDSVolume());
		}
	}

	Flags.Clear(IS_FIRST_FRAME);
}
//---------------------------------------------------------------------

void CAudioEntity::FadeOut(nTime Time)
{
	if (Time > 0.05f)
	{
		Flags.Set(IS_FADING_OUT);
		FadeOutTime = Time;
		FadeOutStartTime = TimeSrv->GetTime();
	}
	else
	{
		Flags.Clear(IS_FADING_OUT);
		Stop();
	}
}
//---------------------------------------------------------------------

inline LONG CAudioEntity::GetDSVolume()
{
	return AsDirectSoundVolume(Volume * AudioSrv->GetMasterVolume(Category));
}
//---------------------------------------------------------------------

} // namespace Audio
