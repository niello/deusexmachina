// Loads keyframe animation clip from KFA (key-framed animation) file.
// Use function declaration instead of header file where you want to call this loader.

#include <Animation/KeyframeClip.h>
#include <Data/BinaryReader.h>
#include <Data/Streams/FileStream.h>

namespace Anim
{
using namespace Data;

bool LoadKeyframeClipFromKFA(Data::CStream& In, PKeyframeClip OutClip)
{
	if (!OutClip.isvalid()) FAIL;

	Data::CBinaryReader Reader(In);

	if (Reader.Read<DWORD>() != 'KFAN') FAIL;

	bool ForBones = Reader.Read<bool>();
	n_assert(!ForBones); // Only for nodes for now

	float Duration = Reader.Read<float>();

	nArray<CKeyframeTrack> Tracks;
	nArray<CSimpleString> TrackMapping;

	DWORD TrackCount = Reader.Read<DWORD>();
	CSimpleString* pMap = TrackMapping.Reserve(TrackCount);
	for (DWORD i = 0; i < TrackCount; ++i)
	{
		nString NodePath;
		Reader.Read(NodePath); //!!!need CSimpleString reader method!
		*pMap++ = NodePath.Get();

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
	Data::CFileStream File;
	return File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL) &&
		LoadKeyframeClipFromKFA(File, OutClip);
}
//---------------------------------------------------------------------

}