#include "ShaderLibraryLoaderSLB.h"

#include <Render/ShaderLibrary.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CShaderLibraryLoaderSLB, 'LSLB', Resources::CResourceLoader);

const Core::CRTTI& CShaderLibraryLoaderSLB::GetResultType() const
{
	return Render::CShaderLibrary::RTTI;
}
//---------------------------------------------------------------------

bool CShaderLibraryLoaderSLB::Load(CResource& Resource)
{
	//???!!!setup stream outside loaders based on URI?!
	const char* pURI = Resource.GetUID().CStr();
	IO::PStream File = IOSrv->CreateStream(pURI);
	if (!File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
	IO::CBinaryReader Reader(*File);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'SLIB') FAIL;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) FAIL;

	U32 ShaderCount;
	if (!Reader.Read(ShaderCount)) FAIL;

	Render::PShaderLibrary ShaderLibrary = n_new(Render::CShaderLibrary);

	ShaderLibrary->TOC.SetSize(ShaderCount);
	for (U32 i = 0; i < ShaderCount; ++i)
	{
		Render::CShaderLibrary::CRecord& Rec = ShaderLibrary->TOC[i];
		if (!Reader.Read(Rec.ID)) FAIL;
		if (!Reader.Read(Rec.Offset)) FAIL;
		if (!Reader.Read(Rec.Size)) FAIL;
	}

	Resource.Init(ShaderLibrary.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}
