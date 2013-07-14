// Loads keyframe animation clip from KFA (key-framed animation) file.
// Use function declaration instead of header file where you want to call this loader.

#include <Animation/KeyframeClip.h>
#include <IO/BinaryReader.h>
#include <IO/Streams/FileStream.h>

namespace Anim
{
using namespace Data;

bool LoadKeyframeClipFromKFA(IO::CStream& In, PKeyframeClip OutClip)
{
	if (!OutClip.IsValid()) FAIL;

	IO::CBinaryReader Reader(In);

	if (Reader.Read<DWORD>() != 'KFAN') FAIL;

	float Duration = Reader.Read<float>();

	CArray<CKeyframeTrack> Tracks;
	CArray<CStrID> TrackMapping;

	DWORD TrackCount = Reader.Read<DWORD>();
	CStrID* pMap = TrackMapping.Reserve(TrackCount);
	for (DWORD i = 0; i < TrackCount; ++i)
	{
		Reader.Read(*pMap++);

		CKeyframeTrack& Track = *Tracks.Reserve(1);
		Track.Channel = (Scene::EChannel)Reader.Read<int>();

		DWORD KeyCount = Reader.Read<DWORD>();
		if (!KeyCount) Reader.Read(Track.ConstValue);
		else
		{
			Track.Keys.SetSize(KeyCount);
			Reader.GetStream().Read(Track.Keys.GetPtr(), KeyCount * sizeof(CKeyframeTrack::CKey));
		}
	}

	//!!!load event tracks!
	CArray<CEventTrack> EventTracks;

	//!!!DBG TMP!
	CEventTrack& ET = *EventTracks.Reserve(1);
	ET.Keys.SetSize(3);
	ET.Keys[0].Time = 0.f;
	ET.Keys[0].EventID = CStrID("OnAnimStart");
	ET.Keys[1].Time = Duration * 0.5f;
	ET.Keys[1].EventID = CStrID("OnAnimHalf");
	ET.Keys[2].Time = Duration;
	ET.Keys[2].EventID = CStrID("OnAnimEnd");

	ET.Keys.Sort();

	return OutClip->Setup(Tracks, TrackMapping, &EventTracks, Duration);
}
//---------------------------------------------------------------------

bool LoadKeyframeClipFromKFA(const nString& FileName, PKeyframeClip OutClip)
{
	IO::CFileStream File;
	return File.Open(FileName, IO::SAM_READ, IO::SAP_SEQUENTIAL) &&
		LoadKeyframeClipFromKFA(File, OutClip);
}
//---------------------------------------------------------------------

}