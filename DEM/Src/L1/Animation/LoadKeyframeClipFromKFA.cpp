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

	nArray<CKeyframeTrack> Tracks;
	nArray<CStrID> TrackMapping;

	DWORD TrackCount = Reader.Read<DWORD>();
	CStrID* pMap = TrackMapping.Reserve(TrackCount);
	for (DWORD i = 0; i < TrackCount; ++i)
	{
		Reader.Read(*pMap++);

		CKeyframeTrack& Track = *Tracks.Reserve(1);
		Track.Channel = (EChannel)Reader.Read<int>();

		DWORD KeyCount = Reader.Read<DWORD>();
		if (!KeyCount) Reader.Read(Track.ConstValue);
		else
		{
			Track.Keys.SetSize(KeyCount);
			Reader.GetStream().Read(Track.Keys.GetPtr(), KeyCount * sizeof(CKeyframeTrack::CKey));
		}
	}

	return OutClip->Setup(Tracks, TrackMapping, Duration);
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