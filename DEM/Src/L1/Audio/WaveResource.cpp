//#include "WaveResource.h"
//
//#include <Audio/Sound.h>
//
//namespace Audio
//{
//__ImplementClassNoFactory(Audio::CWaveResource, Core::CRefCounted);
//__ImplementClass(Audio::CWaveResource);
//
//CWaveResource::~CWaveResource()
//{
//	for (int i = 0; i < Sounds.GetCount(); i++)
//	{
//		Sounds[i]->Release();
//		Sounds[i].invalidate();
//	}
//}
////---------------------------------------------------------------------
//
//bool CWaveResource::IsPlaying() const
//{
//	for (int i = 0; i < Sounds.GetCount(); i++)
//		if (Sounds[i]->IsPlaying()) return true;
//	return false;
//}
////---------------------------------------------------------------------
//
//} // namespace Audio
