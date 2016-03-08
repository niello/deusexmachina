#include "MaterialLoader.h"

#include <Render/Material.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CMaterialLoader, Resources::CResourceLoader);

const Core::CRTTI& CMaterialLoader::GetResultType() const
{
	return Render::CMaterial::RTTI;
}
//---------------------------------------------------------------------

bool CMaterialLoader::Load(CResource& Resource)
{
	const char* pURI = Resource.GetUID().CStr();
	const char* pExt = PathUtils::GetExtension(pURI);
	Data::PParams Desc;
	if (!n_stricmp(pExt, "hrd"))
	{
		Data::CBuffer Buffer;
		if (!IOSrv->LoadFileToBuffer(pURI, Buffer)) FAIL;
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), Desc)) FAIL;
	}
	else if (!n_stricmp(pExt, "prm"))
	{
		IO::PStream File = IOSrv->CreateStream(pURI);
		if (!File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
		IO::CBinaryReader Reader(*File);
		Desc = n_new(Data::CParams);
		if (!Reader.ReadParams(*Desc)) FAIL;
	}
	else FAIL;

	OK;
}
//---------------------------------------------------------------------

}