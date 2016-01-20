#include "SkinInfoLoaderSKN.h"

#include <Render/SkinInfo.h>
#include <Resources/Resource.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CSkinInfoLoaderSKN, 'LSKN', Resources::CResourceLoader);

const Core::CRTTI& CSkinInfoLoaderSKN::GetResultType() const
{
	return Render::CSkinInfo::RTTI;
}
//---------------------------------------------------------------------

bool CSkinInfoLoaderSKN::Load(CResource& Resource)
{
	//???!!!setup stream outside loaders based on URI?!
	const char* pURI = Resource.GetUID().CStr();
	IO::CFileStream File(pURI);
	if (!File.Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
	IO::CBinaryReader Reader(File);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'SKIF') FAIL;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) FAIL;

	U32 BoneCount;
	if (!Reader.Read(BoneCount)) FAIL;

	//!!!may use MMF for bind pose matrices!
	Render::PSkinInfo SkinInfo = n_new(Render::CSkinInfo);
	SkinInfo->Create(BoneCount);

	File.Read(SkinInfo->GetInvBindPoseData(), BoneCount * sizeof(matrix44));

	for (U32 i = 0; i < BoneCount; ++i)
	{
		Render::CBoneInfo& BoneInfo = SkinInfo->GetBoneInfoEditable(i);
		U16 ParentIndex;
		if (!Reader.Read(ParentIndex)) FAIL;
		BoneInfo.ParentIndex = (ParentIndex == (U16)INVALID_INDEX) ? INVALID_INDEX : ParentIndex;
		if (!Reader.Read(BoneInfo.ID)) FAIL;
	}

	Resource.Init(SkinInfo.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}
