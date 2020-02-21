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
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->Open()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'SKIN') return nullptr;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) return nullptr;

	U32 BoneCount;
	if (!Reader.Read(BoneCount)) return nullptr;

	// Skip padding
	Reader.Read<U32>();

	Render::PSkinInfo SkinInfo = n_new(Render::CSkinInfo);
	SkinInfo->Create(BoneCount);

	//!!!may use MMF for bind pose matrices! their offset in a file is aligned-16!
	Stream->Read(SkinInfo->pInvBindPose, BoneCount * sizeof(matrix44));

	for (auto& BoneInfo : SkinInfo->Bones)
	{
		U16 ParentIndex;
		if (!Reader.Read(ParentIndex)) return nullptr;
		BoneInfo.ParentIndex = (ParentIndex == static_cast<U16>(-1)) ? INVALID_INDEX : ParentIndex;
		if (!Reader.Read(BoneInfo.ID)) return nullptr;
	}

	return SkinInfo.Get();
}
//---------------------------------------------------------------------

}
