//#include "WaveResource.h"
//
//#include <Audio/Sound.h>
//
//namespace Audio
//{
//ImplementRTTI(Audio::CWaveResource, Core::CRefCounted);
//ImplementFactory(Audio::CWaveResource);
//
//CWaveResource::~CWaveResource()
//{
//	for (int i = 0; i < Sounds.Size(); i++)
//	{
//		Sounds[i]->Release();
//		Sounds[i].invalidate();
//	}
//}
////---------------------------------------------------------------------
//
//bool CWaveResource::IsPlaying() const
//{
//	for (int i = 0; i < Sounds.Size(); i++)
//		if (Sounds[i]->IsPlaying()) return true;
//	return false;
//}
////---------------------------------------------------------------------
//
//} // namespace Audio
