#include "SkinInfoLoaderSKN.h"

#include <Render/SkinInfo.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{

const Core::CRTTI& CSkinInfoLoaderSKN::GetResultType() const
{
	return Render::CSkinInfo::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CSkinInfoLoaderSKN::CreateResource(CStrID UID)
{
	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'SKIF') return NULL;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) return NULL;

	U32 BoneCount;
	if (!Reader.Read(BoneCount)) return NULL;

	//!!!may use MMF for bind pose matrices!
	Render::PSkinInfo SkinInfo = n_new(Render::CSkinInfo);
	SkinInfo->Create(BoneCount);

	Stream.Read(SkinInfo->GetInvBindPoseData(), BoneCount * sizeof(matrix44));

	for (U32 i = 0; i < BoneCount; ++i)
	{
		Render::CBoneInfo& BoneInfo = SkinInfo->GetBoneInfoEditable(i);
		U16 ParentIndex;
		if (!Reader.Read(ParentIndex)) return NULL;
		BoneInfo.ParentIndex = (ParentIndex == (U16)INVALID_INDEX) ? INVALID_INDEX : ParentIndex;
		if (!Reader.Read(BoneInfo.ID)) return NULL;
	}

	return SkinInfo.Get();
}
//---------------------------------------------------------------------

}
