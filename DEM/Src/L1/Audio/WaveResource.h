//#pragma once
//#ifndef __DEM_L1_WAVE_RESOURCE_H__
//#define __DEM_L1_WAVE_RESOURCE_H__
//
//#include <Core/Object.h>
//#include <kernel/nref.h>
//#include <Data/String.h>
//
//// A wave resource is an entry in a CWaveBank.
//
//namespace Audio
//{
//
//class CWaveResource: public Core::CObject
//{
//	__DeclareClassNoFactory;
//	__DeclareClass(CWaveResource);
//
//private:
//
//	CString					Name;
//	CArray<nRef<nSound3>>	Sounds;
//
//public:
//
//	float					Volume;
//
//	CWaveResource(): Volume(1.0f) {}
//	virtual ~CWaveResource();
//
//	void			SetName(const CString& NewName) { n_assert(NewName.IsValid()); Name = NewName; }
//	const CString&	GetName() const { return Name; }
//	void			AddSoundObject(nSound3* pSound) { n_assert(pSound); Sounds.Add(pSound); }
//	int				GetNumSoundObjects() const { return Sounds.GetCount(); }
//	nSound3*			GetSoundObjectAt(int Idx) const { return Sounds[Idx]; }
//	nSound3*			GetRandomSoundObject() const { return Sounds[rand() % Sounds.GetCount()]; }
//	bool			IsPlaying() const;
//};
//
//}
//
//#endif
