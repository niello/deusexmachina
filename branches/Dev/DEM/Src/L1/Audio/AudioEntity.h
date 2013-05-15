#pragma once
#ifndef __DEM_L1_AUDIO_ENTITY_H__
#define __DEM_L1_AUDIO_ENTITY_H__

#include <Core/RefCounted.h>
#include <Data/Flags.h>
#include <Audio/Audio.h>
#include <Audio/SoundResource.h>
#include <Audio/DSUtil/DSUtil.h>

//???split to 2D & 3D?
// An audio entity is a sound object (instance) in 3D space.
// Hold parameters for a sound instance. Usually a "game object" holds
// one or more of these objects for all the sounds it has to play.
// DirectSound implementation.

namespace Audio
{
class CSoundResource;

class CAudioEntity: public Core::CRefCounted
{
	__DeclareClass(CAudioEntity);

private:

	enum
	{
		IS_ACTIVE		= 0x01,
		IS_FIRST_FRAME	= 0x02,
		IS_FADING_OUT	= 0x04
	};

	Data::CFlags			Flags;

	nTime					FadeOutStartTime;
	nTime					FadeOutTime;
	matrix44				Transform;
	vector3					Velocity;
	DWORD					SoundIdx;
	DS3DBUFFER				DS3DProps;
	float					minDist;
	float					maxDist;
	int						insideConeAngle;
	int						outsideConeAngle;
	float					coneOutsideVolume;
	bool					Props3DDirty; //???split to sound & sound3d?

	LONG			GetDSVolume();
	DS3DBUFFER*		GetDS3DProps();

	static LONG		AsDirectSoundVolume(float vol);

public:

	nString					RsrcName;

	float					Volume;
	int						Priority;
	ESoundCategory			Category;

	//???where should be stored, rsrc or instance?
	int				NumTracks;
	bool			Ambient;
	bool			Streaming;
	bool			Looping;
	//NumTracks(5),
	//Ambient(false),
	//Streaming(false),
	//Looping(false)

	CAudioEntity();
	virtual ~CAudioEntity() { if (Flags.Is(IS_ACTIVE)) Deactivate(); }

	virtual void	Activate();
	virtual void	Deactivate();

	void			Start();
	void			Stop();
	void			Update();
	bool			IsPlaying() const;
	void			FadeOut(nTime Time);

	
	//CDSSound*			GetCSoundPtr() const { return refRsrc->GetCSoundPtr(); }
	//CSoundResource*	GetSoundResource() const { return refRsrc.CStr(); }

	void			SetTransform(const matrix44& m) { Transform = m; Props3DDirty = true; }
	const matrix44&	GetTransform() const { return Transform; }
	void			SetVelocity(const vector3& v) { Velocity = v; Props3DDirty = true; }
	const vector3&	GetVelocity() const { return Velocity; }
	void			SetMinDist(float d) { minDist = d; Props3DDirty = true; }
	float			GetMinDist() const { return minDist; }
	void			SetMaxDist(float d) { maxDist = d; Props3DDirty = true; }
	float			GetMaxDist() const { return maxDist; }
	void			SetInsideConeAngle(int a) { insideConeAngle = a; Props3DDirty = true; }
	int				GetInsideConeAngle() const { return insideConeAngle; }
	void			SetOutsideConeAngle(int a) { outsideConeAngle = a; Props3DDirty = true; }
	int				GetOutsideConeAngle() const { return outsideConeAngle; }
	void			SetConeOutsideVolume(float v) { coneOutsideVolume = v; Props3DDirty = true; }
	float			GetConeOutsideVolume() const { return coneOutsideVolume; }

	void			CopyFrom(const CAudioEntity& Other);
};
//---------------------------------------------------------------------

__RegisterClassInFactory(CAudioEntity);

inline void CAudioEntity::CopyFrom(const CAudioEntity& Other)
{
	*this = Other;
	Props3DDirty = true;
}
//---------------------------------------------------------------------

inline bool CAudioEntity::IsPlaying() const
{
	// At the first frame the sound has been started but not updated, dsound will return false,
	// but the sound will start in the next frame
	if (Flags.Is(IS_FIRST_FRAME)) OK;
	if (SoundIdx == -1) return false;
	return false; // (Streaming ? GetCSoundPtr()->IsSoundPlaying() : GetCSoundPtr()->IsSoundPlaying(SoundIdx)) != 0;
}
//---------------------------------------------------------------------

// Convert a linear Volume between 0.0f and 1.0f into a Dezibel-based DirectSound Volume
inline LONG CAudioEntity::AsDirectSoundVolume(float vol)
{
	float ScaledVolume = (vol > 0.f) ? (.4f + vol * .6f) : 0.f;
	return (LONG)(DSBVOLUME_MIN + ((DSBVOLUME_MAX - DSBVOLUME_MIN) * ScaledVolume));
}
//---------------------------------------------------------------------

inline DS3DBUFFER* CAudioEntity::GetDS3DProps()
{
	if (Props3DDirty)
	{
		Props3DDirty = false;
		DS3DProps.vPosition.x = Transform.M41;
		DS3DProps.vPosition.y = Transform.M42;
		DS3DProps.vPosition.z = Transform.M43;
		DS3DProps.vVelocity.x = Velocity.x;
		DS3DProps.vVelocity.y = Velocity.y;
		DS3DProps.vVelocity.z = Velocity.z;
		DS3DProps.dwInsideConeAngle = (DWORD)insideConeAngle;
		DS3DProps.dwOutsideConeAngle = (DWORD)outsideConeAngle;
		DS3DProps.vConeOrientation.x = Transform.M31;
		DS3DProps.vConeOrientation.y = Transform.M32;
		DS3DProps.vConeOrientation.z = Transform.M33;
		DS3DProps.lConeOutsideVolume = AsDirectSoundVolume(coneOutsideVolume);
		DS3DProps.flMinDistance = minDist;
		DS3DProps.flMaxDistance = maxDist;
		DS3DProps.dwMode = DS3DMODE_NORMAL;
	}
	return &(DS3DProps);
}
//---------------------------------------------------------------------

}

#endif
