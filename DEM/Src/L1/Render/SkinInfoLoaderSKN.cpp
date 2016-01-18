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

	Render::PSkinInfo SkinInfo = n_new(Render::CSkinInfo);

	//File.Read(SkinInfo->pInvBindPose, BoneCount * sizeof(matrix44));

	//???make members public to avoid copying bind pose twice?
	//!!!may use MMF for bind pose matrices!
	//SkinInfo->Create(InitData);

	Resource.Init(SkinInfo.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}
