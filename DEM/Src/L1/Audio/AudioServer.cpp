#include "AudioServer.h"

#include "WaveResource.h"
#include "WaveBank.h"
#include "AudioEntity.h"
#include <Audio/Event/PlaySound.h>
#include <Audio/DSUtil/DSUtil.h>
#include <Events/EventServer.h>
#include <Time/TimeServer.h>
#include <Core/CoreServer.h>

namespace Audio
{
__ImplementClassNoFactory(Audio::CAudioServer, Core::CRefCounted);

__ImplementSingleton(Audio::CAudioServer);

CAudioServer::CAudioServer():
	_IsOpen(false),
	_IsMuted(false),
	pDS(NULL),
	pDSListener(NULL),
	LastStreamUpdate(0.f),
	NoSoundDevice(false),
	ListenerRollOffFactor(1.0f),
	ListenerDopplerFactor(1.0f)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CAudioServer::~CAudioServer()
{
	n_assert(!pDS);
	n_assert(!pDSListener);
	n_assert(!_IsOpen);
	//n_assert(!WaveBank.IsValid());
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CAudioServer::Open()
{
	n_assert(!_IsOpen);
	n_assert(!pDS);
	n_assert(!pDSListener);

	float CurrTime = (float)TimeSrv->GetTime();
	for (int i = 0; i < SoundCategoryCount; ++i)
	{
		VolumeRec[i].Volume = 1.f;
		VolumeRec[i].ChangeTime = CurrTime;
		VolumeRec[i].IsDirty = true;
	}

	HWND hWnd = NULL;
	CoreSrv->GetGlobal("hwnd", (int&)hWnd);
	NoSoundDevice =
		FAILED(DirectSoundCreate8(NULL, &pDS, NULL)) ||
		FAILED(pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY));
	if (NoSoundDevice)
	{
		n_printf("DirectSound device initialization failed, no sound device");
		_IsOpen = true;
		return false;
	}

	n_assert(pDS);

	DSBUFFERDESC DSBufDesc;
	ZeroMemory(&DSBufDesc, sizeof(DSBUFFERDESC));
	DSBufDesc.dwSize = sizeof(DSBUFFERDESC);
	DSBufDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;

	LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;
	if (FAILED(pDS->CreateSoundBuffer(&DSBufDesc, &pDSBPrimary, NULL)))
		n_error("AudioSrv::Open(): pDS->CreateSoundBuffer(Primary) failed");

	//???channels+samples+bits to settings HRD?
	WAVEFORMATEX WaveFmt;
	ZeroMemory(&WaveFmt, sizeof(WAVEFORMATEX));
	WaveFmt.wFormatTag = (WORD)WAVE_FORMAT_PCM;
	WaveFmt.nChannels = 2;
	WaveFmt.nSamplesPerSec = 22050;
	WaveFmt.wBitsPerSample = 16;
	WaveFmt.nBlockAlign = (WORD)((WaveFmt.wBitsPerSample >> 3) * WaveFmt.nChannels);
	WaveFmt.nAvgBytesPerSec = (DWORD)(WaveFmt.nSamplesPerSec * WaveFmt.nBlockAlign);

	if (FAILED(pDSBPrimary->SetFormat(&WaveFmt)))
		n_error("AudioSrv::Open(): pDSBPrimary->SetFormat failed");

	if (FAILED(pDSBPrimary->QueryInterface(IID_IDirectSound3DListener, (VOID**)&pDSListener)))
		n_error("AudioSrv::Open(): pDSBPrimary->QueryInterface failed");

	SAFE_RELEASE(pDSBPrimary);

	SUBSCRIBE_NEVENT(PlaySound, CAudioServer, OnPlaySound);

	_IsOpen = true;
	return true;
}
//---------------------------------------------------------------------

void CAudioServer::Close()
{
	n_assert(_IsOpen);

	UNSUBSCRIBE_EVENT(PlaySound);

	while (Entities.GetCount()) RemoveEntity(Entities.Back());

	//if (WaveBank.IsValid()) CloseWaveBank();

	SAFE_RELEASE(pDSListener);
	SAFE_RELEASE(pDS);

	_IsOpen = false;
}
//---------------------------------------------------------------------

void CAudioServer::AttachEntity(CAudioEntity* pEntity)
{
	n_assert(pEntity);
	pEntity->Activate();
	Entities.Add(pEntity);
}
//---------------------------------------------------------------------

void CAudioServer::RemoveEntity(CAudioEntity* pEntity)
{
	n_assert(pEntity);
	pEntity->Deactivate();
	CArray<PAudioEntity>::CIterator itEntity = Entities.Find(pEntity);
	n_assert(itEntity);
	Entities.Remove(itEntity);
}
//---------------------------------------------------------------------

//bool CAudioServer::OpenWaveBank(const CString& Name)
//{
	//n_assert(!WaveBank.IsValid());
	//WaveBank = CWaveBank::CreateInstance();
	//WaveBank->SetFilename(Name);
	//return WaveBank->Open();
//	FAIL;
//}
////---------------------------------------------------------------------

//void CAudioServer::CloseWaveBank()
//{
//	n_assert(WaveBank.IsValid());
//	WaveBank->Close();
//	WaveBank = NULL;
//}
////---------------------------------------------------------------------

void CAudioServer::Trigger()
{
	if (NoSoundDevice) return;

	float CurrTime = (float)TimeSrv->GetTime();
	float Diff = CurrTime - LastStreamUpdate;

	if (Diff > 0.02f || Diff <= 0.f)
	{
		LastStreamUpdate = CurrTime;

		// For each resource
				//CDSStreamingSound* pSound = (CDSStreamingSound*)pSndRsrc->GetCSoundPtr();
				//if (pSound->IsSoundPlaying() && pSound->CheckStreamUpdate())
				//	pSound->HandleWaveStreamNotification(pSndRsrc->Looping);
	}

	for (int i = 0; i < Entities.GetCount(); i++) Entities[i]->Update();

	//for (int Cat = 0; Cat < SoundCategoryCount; Cat++)
	//	if (VolumeRec[Cat].IsDirty)
	//	{
	//		VolumeRec[Cat].IsDirty = false;
	//		nRoot* pRsrcPool = nResourceServer::Instance()->GetResourcePool(nResource::SoundInstance);
	//		for (CDSSound* cur = (CDSSound*)pRsrcPool->GetHead(); cur; cur = (CDSSound*)cur->GetSucc())
	//			if (pSound->Category == Cat && pSound->IsPlaying()) pSound->Update();
	//	}

	//!!!can cache not to construct every frame!
	n_assert(pDSListener);
	DS3DLISTENER DSListenerProps;
	DSListenerProps.dwSize = sizeof(DSListenerProps);	
	DSListenerProps.vPosition.x = ListenerTransform.M41;
	DSListenerProps.vPosition.y = ListenerTransform.M42;
	DSListenerProps.vPosition.z = ListenerTransform.M43;
	DSListenerProps.vVelocity.x = ListenerVelocity.x;
	DSListenerProps.vVelocity.y = ListenerVelocity.y;
	DSListenerProps.vVelocity.z = ListenerVelocity.z;
	DSListenerProps.vOrientFront.x = ListenerTransform.M31;
	DSListenerProps.vOrientFront.y = ListenerTransform.M32;
	DSListenerProps.vOrientFront.z = ListenerTransform.M33;
	DSListenerProps.vOrientTop.x = ListenerTransform.M21;
	DSListenerProps.vOrientTop.y = ListenerTransform.M22;
	DSListenerProps.vOrientTop.z = ListenerTransform.M23;
	DSListenerProps.flDistanceFactor = 1.0f;
	DSListenerProps.flRolloffFactor = ListenerRollOffFactor;
	DSListenerProps.flDopplerFactor = ListenerDopplerFactor;

	n_assert(SUCCEEDED(pDSListener->SetAllParameters(&DSListenerProps, DS3D_IMMEDIATE)));

	n_assert(_IsOpen);
}
//---------------------------------------------------------------------

void CAudioServer::PlaySoundEffect(const CString& FXName, const vector3& Pos, const vector3& Vel, float Volume)
{
	n_assert(FXName.IsValid());

	/*
	if (WaveBank)
	{
		CWaveResource* pResource = WaveBank->FindResource(FXName);
		if (pResource)
		{
			CDSSound* pSound = pResource->GetRandomSoundObject();
			n_assert(pSound);

			matrix44 ListenerTransform;
			ListenerTransform.translate(Pos);
			pSound->Transform = ListenerTransform;
			pSound->Velocity = Vel;
			pSound->SetVolume(pResource->Volume * Volume);

			nAudioServer3::Instance()->StartSound(pSound);
		}
	}
	*/
}
//---------------------------------------------------------------------

bool CAudioServer::OnPlaySound(const Events::CEventBase& Event)
{
	const Event::PlaySound& e = (const Event::PlaySound&)Event;
	PlaySoundEffect(e.Name, e.Position, e.Velocity, e.Volume);
	OK;
}
//---------------------------------------------------------------------

void CAudioServer::SetMasterVolume(ESoundCategory Cat, float Volume)
{
	VolumeRec[Cat].Volume = Volume;
	VolumeRec[Cat].IsDirty = true;
	VolumeRec[Cat].ChangeTime = (float)TimeSrv->GetTime();
}
//---------------------------------------------------------------------

void CAudioServer::Mute()
{
	n_assert(!_IsMuted);
	for (int i = 0; i < SoundCategoryCount; ++i)
	{
		VolumeRec[i].MutedVolume = VolumeRec[i].Volume;
		VolumeRec[i].Volume = 0.f;
	}
	_IsMuted = true;
}
//---------------------------------------------------------------------

void CAudioServer::Unmute()
{
	n_assert(_IsMuted);
	for (int i = 0; i < SoundCategoryCount; ++i)
		SetMasterVolume((ESoundCategory)i, VolumeRec[i].MutedVolume);
	_IsMuted = false;
}
//---------------------------------------------------------------------

const char* SoundCategoryToString(ESoundCategory Cat)
{
	switch (Cat)
	{
		case Effect:	return "effect";
		case Music:		return "music";
		case Speech:	return "speech";
		case Ambient:	return "ambient";
		default: n_error("nAudioServer3: Invalid Category: %i!", Cat); return "";
	}
}
//---------------------------------------------------------------------

ESoundCategory StringToSoundCategory(const char* pStr)
{
	if (!strcmp(pStr, "effect")) return Effect;
	if (!strcmp(pStr, "music")) return Music;
	if (!strcmp(pStr, "speech")) return Speech;
	if (!strcmp(pStr, "ambient")) return Ambient;
	n_error("nAudioServer3: Invalid category string '%s'.\n", pStr);
	return InvalidSoundCategory;
}
//---------------------------------------------------------------------

} // namespace Audio
