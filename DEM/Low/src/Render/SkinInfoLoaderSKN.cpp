#include "SkinInfoLoaderSKN.h"

#include <Render/SkinInfo.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const Core::CRTTI& CSkinInfoLoaderSKN::GetResultType() const
{
	return Render::CSkinInfo::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CSkinInfoLoaderSKN::CreateResource(CStrID UID)
{
	if (!pResMgr) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'SKIF') return nullptr;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) return nullptr;

	U32 BoneCount;
	if (!Reader.Read(BoneCount)) return nullptr;

	//!!!may use MMF for bind pose matrices!
	Render::PSkinInfo SkinInfo = n_new(Render::CSkinInfo);
	SkinInfo->Create(BoneCount);

	Stream->Read(SkinInfo->GetInvBindPoseData(), BoneCount * sizeof(matrix44));

	for (U32 i = 0; i < BoneCount; ++i)
	{
		Render::CBoneInfo& BoneInfo = SkinInfo->GetBoneInfoEditable(i);
		U16 ParentIndex;
		if (!Reader.Read(ParentIndex)) return nullptr;
		BoneInfo.ParentIndex = (ParentIndex == (U16)INVALID_INDEX) ? INVALID_INDEX : ParentIndex;
		if (!Reader.Read(BoneInfo.ID)) return nullptr;
	}

	return SkinInfo.Get();
}
//---------------------------------------------------------------------

}
