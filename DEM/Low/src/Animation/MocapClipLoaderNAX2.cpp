#include "MocapClipLoaderNAX2.h"

#include <Animation/MocapClip.h>
#include <Render/SkinInfo.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
#pragma pack(push, 1)
struct CNAX2Header
{
	U32 magic;         // NAX2
	U32 numGroups;     // number of groups in file
	U32 numKeys;       // number of keys in file
};

struct CNAX2Group
{
	U32 numCurves;         // number of curves in group
	U32 startKey;          // first key index
	U32 numKeys;           // number of keys in group
	U32 keyStride;         // key stride in key pool
	float keyTime;           // key duration
	float fadeInFrames;      // number of fade in frames
	U32 loopType;          // nAnimation::LoopType
};

struct CNAX2Curve
{
	U32 ipolType;          // nAnimation::Curve::IpolType
	I32 firstKeyIndex;     // index of first curve key in key pool (-1 if collapsed!)
	U32 isAnimated;        // flag, if the curve's joint is animated
	vector4 collapsedKey;   // the key value if this is a collapsed curve
};
#pragma pack(pop)

CMocapClipLoaderNAX2::CMocapClipLoaderNAX2(IO::CIOServer* pIOServer) : CResourceLoader(pIOServer) {}
CMocapClipLoaderNAX2::~CMocapClipLoaderNAX2() {}

const Core::CRTTI& CMocapClipLoaderNAX2::GetResultType() const
{
	return Anim::CMocapClip::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CMocapClipLoaderNAX2::CreateResource(CStrID UID)
{
	if (ReferenceSkinInfo.IsNullPtr()) return NULL;

	const char* pSubId;
	IO::PStream Stream = OpenStream(UID, pSubId);
	if (!Stream) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	CNAX2Header Header;
	if (!Reader.Read(Header) || Header.magic != 'NAX2') return NULL;

	CNAX2Group Group;
	if (!Reader.Read(Group)) return NULL;

	UPTR TotalCurves = Group.numCurves;
	for (UPTR i = 1; i < Header.numGroups; ++i)
	{
		CNAX2Group TmpGroup;
		if (!Reader.Read(TmpGroup)) return NULL;
		TotalCurves += TmpGroup.numCurves;
	}

	const vector4 NoScaling(1.f, 1.f, 1.f, 0.f);
	const vector4 NoRotation(0.f, 0.f, 0.f, 1.f);
	const vector4 NoTranslation(0.f, 0.f, 0.f, 0.f);

	CArray<Anim::CMocapTrack> Tracks;
	CArray<CStrID> TrackMapping;
	for (UPTR i = 0; i < Group.numCurves; ++i)
	{
		CNAX2Curve Curve;
		if (!Reader.Read(Curve)) return NULL;
		if (!Curve.isAnimated) continue;
		
		Scene::ETransformChannel Channel;
		switch (i % 3)
		{
			case 0: Channel = Scene::Tfm_Translation; break;
			case 1: Channel = Scene::Tfm_Rotation; break;
			case 2: Channel = Scene::Tfm_Scaling; break;
		}

		// Skip unused tracks
		if (Curve.firstKeyIndex == -1)
		{
			if ((Channel == Scene::Tfm_Scaling &&
				Curve.collapsedKey.isequal(NoScaling, 0.000001f)) ||
				(Channel == Scene::Tfm_Rotation &&
				Curve.collapsedKey.isequal(NoRotation, 0.000001f)) ||
				(Channel == Scene::Tfm_Translation &&
				Curve.collapsedKey.isequal(NoTranslation, 0.000001f)))
			{
				continue;
			}
		}

		CString RelNodePath;
		UPTR BoneIdx = i / 3;
		const Render::CBoneInfo* pBoneInfo = &ReferenceSkinInfo->GetBoneInfo(BoneIdx);
		RelNodePath += pBoneInfo->ID.CStr();
		while (pBoneInfo->ParentIndex != INVALID_INDEX)
		{
			pBoneInfo = &ReferenceSkinInfo->GetBoneInfo(pBoneInfo->ParentIndex);
			RelNodePath = '.' + RelNodePath;
			RelNodePath = pBoneInfo->ID.CStr() + RelNodePath;
		}

		Anim::CMocapTrack& Track = *Tracks.Reserve(1);
		Track.FirstKey = Curve.firstKeyIndex;
		Track.ConstValue = Curve.collapsedKey;
		Track.Channel = Channel;

		TrackMapping.Add(CStrID(RelNodePath.CStr()));
	}

	for (UPTR i = Group.numCurves; i < TotalCurves; ++i)
		Reader.Read<CNAX2Curve>();

	UPTR KeyCount = Group.numKeys * Group.keyStride;
	vector4* pKeys = n_new_array(vector4, KeyCount);
	Stream->Read(pKeys, KeyCount * sizeof(vector4));

	//???load directly to Clip fields?
	Anim::PMocapClip Clip = n_new(Anim::CMocapClip);
	Clip->Setup(Tracks, TrackMapping, NULL, pKeys, Group.numKeys, Group.keyStride, Group.keyTime);

	return Clip.Get();
}
//---------------------------------------------------------------------

}
