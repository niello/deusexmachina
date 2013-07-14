#pragma once
#ifndef __DEM_L1_AUDIO_SERVER_H__
#define __DEM_L1_AUDIO_SERVER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <Events/EventsFwd.h>
#include <Audio/Audio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>

// The AudioServer object is the central object of the audio subsystem.
// DirectSound implementation.

class CSoundManager;

namespace Audio
{
typedef Ptr<class CAudioEntity> PAudioEntity;
typedef Ptr<class CWaveBank> PWaveBank;
class CSoundResource;

#define AudioSrv Audio::CAudioServer::Instance()

class CAudioServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CAudioServer);

private:

	bool						_IsOpen;
	bool						_IsMuted;
	//PWaveBank					WaveBank;
	CArray<PAudioEntity>		Entities;

	struct CVolumeRecord
	{
		float	Volume;
		float	MutedVolume;
		float	ChangeTime;
		bool	IsDirty;
	}							VolumeRec[SoundCategoryCount];

	LPDIRECTSOUND8				pDS;
	IDirectSound3DListener8*	pDSListener;
	float						LastStreamUpdate;
	bool						NoSoundDevice;

	DECLARE_EVENT_HANDLER(PlaySound, OnPlaySound);

public:

	matrix44	ListenerTransform;
	vector3		ListenerVelocity;
	float		ListenerRollOffFactor;
	float		ListenerDopplerFactor;

	CAudioServer();
	virtual ~CAudioServer();

	bool			Open();
	void			Close();
	void			Trigger();

	//bool			OpenWaveBank(const nString& Name);
	//void			CloseWaveBank();
	//CWaveBank*		GetWaveBank() const { return WaveBank.GetUnsafe(); }
	
	void			PlaySoundEffect(const nString& FXName, const vector3& Pos, const vector3& Vel, float Volume);
	
	void			AttachEntity(CAudioEntity* e);
	void			RemoveEntity(CAudioEntity* e);
	int				GetNumEntities() const { return Entities.GetCount(); }
	CAudioEntity*	GetEntityAt(int Idx) const { return Entities[Idx]; }

	void			SetMasterVolume(ESoundCategory Cat, float Volume);
	float			GetMasterVolume(ESoundCategory Cat) const { return VolumeRec[Cat].Volume; }
	void			Mute();
	void			Unmute();
	bool			IsOpen() const { return _IsOpen; }
	bool			IsMuted() const { return _IsMuted; }

	inline LPDIRECTSOUND8 GetDirectSound() const { return pDS; }
};

/*
CDSSound* CAudioServer::CreateSoundFromResourceName(const nString& Name)
{
	n_assert(WaveBank.IsValid());
	CWaveResource* pResource = WaveBank->FindResource(Name);
	if (pResource)
	{
		CDSSound* pOriginalSound = pResource->GetSoundObjectAt(0);
		n_assert(pOriginalSound);
		CDSSound* pNewSound = nAudioServer3::Instance()->NewSound();
		pNewSound->SetFilename(pOriginalSound->GetFilename());
		pNewSound->CopySoundAttrsFrom(pOriginalSound);
		pNewSound->SetVolume(pResource->Volume);
		return pNewSound;
	}
	else return NULL;
}
//---------------------------------------------------------------------
*/

}

#endif
