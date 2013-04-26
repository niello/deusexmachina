// Loads first animation group of .nax2 file as mocap clip
// Use function declaration instead of header file where you want to call this loader.

#include <Animation/MocapClip.h>
#include <Data/BinaryReader.h>
#include <Data/Streams/FileStream.h>

namespace Anim
{
#pragma pack(push, 1)
struct CNAX2Header
{
	uint magic;         // NAX2
	uint numGroups;     // number of groups in file
	uint numKeys;       // number of keys in file
};

struct CNAX2Group
{
	uint numCurves;         // number of curves in group
	uint startKey;          // first key index
	uint numKeys;           // number of keys in group
	uint keyStride;         // key stride in key pool
	float keyTime;           // key duration
	float fadeInFrames;      // number of fade in frames
	uint loopType;          // nAnimation::LoopType
};

struct CNAX2Curve
{
	uint ipolType;          // nAnimation::Curve::IpolType
	int firstKeyIndex;     // index of first curve key in key pool (-1 if collapsed!)
	uint isAnimated;        // flag, if the curve's joint is animated
	vector4 collapsedKey;   // the key value if this is a collapsed curve
};
#pragma pack(pop)

bool LoadMocapClipFromNAX2(Data::CStream& In, const nDictionary<int, CStrID>& BoneToNode, PMocapClip OutClip)
{
	if (!OutClip.isvalid()) FAIL;

	Data::CBinaryReader Reader(In);

	CNAX2Header Header;
	Reader.Read(Header);
	if (Header.magic != 'NAX2') FAIL;

	CNAX2Group Group;
	Reader.Read(Group);

	DWORD TotalCurves = Group.numCurves;
	for (uint i = 1; i < Header.numGroups; ++i)
	{
		CNAX2Group TmpGroup;
		Reader.Read(TmpGroup);
		TotalCurves += TmpGroup.numCurves;
	}

	nArray<CMocapTrack> Tracks;
	nArray<CStrID> TrackMapping;
	for (uint i = 0; i < Group.numCurves; ++i)
	{
		CNAX2Curve Curve;
		Reader.Read(Curve);
		if (!Curve.isAnimated) continue;
		
		EChannel Channel;
		switch (i % 3)
		{
			case 0: Channel = Chnl_Translation; break;
			case 1: Channel = Chnl_Rotation; break;
			case 2: Channel = Chnl_Scaling; break;
		}

		// Skip unused tracks
		if (Curve.firstKeyIndex == -1)
		{
			if ((Channel == Chnl_Scaling &&
				Curve.collapsedKey.x == 1.f &&
				Curve.collapsedKey.y == 1.f &&
				Curve.collapsedKey.z == 1.f) ||
				(Channel == Chnl_Rotation &&
				Curve.collapsedKey.x == 0.f &&
				Curve.collapsedKey.y == 0.f &&
				Curve.collapsedKey.z == 0.f &&
				Curve.collapsedKey.w == 1.f) ||
				(Channel == Chnl_Translation &&
				Curve.collapsedKey.x == 0.f &&
				Curve.collapsedKey.y == 0.f &&
				Curve.collapsedKey.z == 0.f))
			{
				continue;
			}
		}

		int RemapIdx = BoneToNode.FindIndex(i / 3);
		if (RemapIdx == INVALID_INDEX) continue; // No such bone in a target skeleton
		CStrID RelNodePath = BoneToNode.ValueAtIndex(RemapIdx);

		CMocapTrack& Track = *Tracks.Reserve(1);
		Track.FirstKey = Curve.firstKeyIndex;
		Track.ConstValue = Curve.collapsedKey;
		Track.Channel = Channel;
		TrackMapping.Append(RelNodePath);
	}

	for (uint i = Group.numCurves; i < TotalCurves; ++i)
		Reader.Read<CNAX2Curve>();

	DWORD KeyCount = Group.numKeys * Group.keyStride;
	vector4* pKeys = n_new_array(vector4, KeyCount);
	Reader.GetStream().Read(pKeys, KeyCount * sizeof(vector4));

	return OutClip->Setup(Tracks, TrackMapping, pKeys, Group.numKeys, Group.keyStride, Group.keyTime);
}
//---------------------------------------------------------------------

bool LoadMocapClipFromNAX2(const nString& FileName, const nDictionary<int, CStrID>& BoneToNode, PMocapClip OutClip)
{
	Data::CFileStream File;
	return File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL) &&
		LoadMocapClipFromNAX2(File, BoneToNode, OutClip);
}
//---------------------------------------------------------------------

}