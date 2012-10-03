//#pragma once
//#ifndef __DEM_L1_WAVE_RESOURCE_H__
//#define __DEM_L1_WAVE_RESOURCE_H__
//
//#include <Core/RefCounted.h>
//#include <kernel/nref.h>
//#include <util/nstring.h>
//
//// A wave resource is an entry in a CWaveBank.
//
//namespace Audio
//{
//
//class CWaveResource: public Core::CRefCounted
//{
//	DeclareRTTI;
//	DeclareFactory(CWaveResource);
//
//private:
//
//	nString					Name;
//	nArray<nRef<nSound3>>	Sounds;
//
//public:
//
//	float					Volume;
//
//	CWaveResource(): Volume(1.0f) {}
//	virtual ~CWaveResource();
//
//	void			SetName(const nString& NewName) { n_assert(NewName.IsValid()); Name = NewName; }
//	const nString&	GetName() const { return Name; }
//	void			AddSoundObject(nSound3* pSound) { n_assert(pSound); Sounds.Append(pSound); }
//	int				GetNumSoundObjects() const { return Sounds.Size(); }
//	nSound3*			GetSoundObjectAt(int Idx) const { return Sounds[Idx]; }
//	nSound3*			GetRandomSoundObject() const { return Sounds[rand() % Sounds.Size()]; }
//	bool			IsPlaying() const;
//};
//
//RegisterFactory(CWaveResource);
//
//}
//
//#endif
