#include "KeyframeClipLoaderKFA.h"

#include <Animation/KeyframeClip.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
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

bool CKeyframeClipLoaderKFA::Load(CResource& Resource)
{
	//???!!!setup stream outside loaders based on URI?!
	const char* pURI = Resource.GetUID().CStr();
	IO::PStream File = IOSrv->CreateStream(pURI);
	if (!File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
	IO::CBinaryReader Reader(*File);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'KFAN') FAIL;

	float Duration;
	if (!Reader.Read(Duration)) FAIL;

	U32 TrackCount;
	if (!Reader.Read(TrackCount)) FAIL;

	CArray<Anim::CKeyframeTrack> Tracks;
	CArray<CStrID> TrackMapping;

	Anim::CKeyframeTrack* pTrack = Tracks.Reserve(TrackCount);
	CStrID* pMap = TrackMapping.Reserve(TrackCount);
	for (UPTR i = 0; i < TrackCount; ++i, ++pTrack, ++pMap)
	{
		Reader.Read(*pMap);

		pTrack->Channel = (Scene::ETransformChannel)Reader.Read<int>();

		U32 KeyCount;
		if (!Reader.Read(KeyCount)) FAIL;

		if (!KeyCount) Reader.Read(pTrack->ConstValue);
		else
		{
			pTrack->Keys.SetSize(KeyCount);
			File->Read(pTrack->Keys.GetPtr(), KeyCount * sizeof(Anim::CKeyframeTrack::CKey));
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

	Resource.Init(Clip.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}
