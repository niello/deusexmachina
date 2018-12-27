#include "KeyframeClipLoaderKFA.h"

#include <Animation/KeyframeClip.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CKeyframeClipLoaderKFA, 'LKFA', Resources::CResourceLoader);

const Core::CRTTI& CKeyframeClipLoaderKFA::GetResultType() const
{
	return Anim::CKeyframeClip::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CKeyframeClipLoaderKFA::Load(IO::CStream& Stream)
{
	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'KFAN') return NULL;

	float Duration;
	if (!Reader.Read(Duration)) return NULL;

	U32 TrackCount;
	if (!Reader.Read(TrackCount)) return NULL;

	CArray<Anim::CKeyframeTrack> Tracks;
	CArray<CStrID> TrackMapping;

	Anim::CKeyframeTrack* pTrack = Tracks.Reserve(TrackCount);
	CStrID* pMap = TrackMapping.Reserve(TrackCount);
	for (UPTR i = 0; i < TrackCount; ++i, ++pTrack, ++pMap)
	{
		Reader.Read(*pMap);

		pTrack->Channel = (Scene::ETransformChannel)Reader.Read<int>();

		U32 KeyCount;
		if (!Reader.Read(KeyCount)) return NULL;

		if (!KeyCount) Reader.Read(pTrack->ConstValue);
		else
		{
			pTrack->Keys.SetSize(KeyCount);
			Stream.Read(pTrack->Keys.GetPtr(), KeyCount * sizeof(Anim::CKeyframeTrack::CKey));
		}
	}

	//!!!load event tracks!
	CArray<Anim::CEventTrack> EventTracks;

	//!!!DBG TMP!
	Anim::CEventTrack& ET = *EventTracks.Reserve(1);
	ET.Keys.SetSize(3);
	ET.Keys[0].Time = 0.f;
	ET.Keys[0].EventID = CStrID("OnAnimStart");
	ET.Keys[1].Time = Duration * 0.5f;
	ET.Keys[1].EventID = CStrID("OnAnimHalf");
	ET.Keys[2].Time = Duration;
	ET.Keys[2].EventID = CStrID("OnAnimEnd");

	ET.Keys.Sort();

	//???load directly to Clip fields?
	Anim::PKeyframeClip Clip = n_new(Anim::CKeyframeClip);
	Clip->Setup(Tracks, TrackMapping, &EventTracks, Duration);

	return Clip.GetUnsafe();
}
//---------------------------------------------------------------------

}
