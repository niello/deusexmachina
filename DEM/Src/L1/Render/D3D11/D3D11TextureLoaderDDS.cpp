#include "D3D11TextureLoaderDDS.h"

#include <Render/D3D11/D3D11Texture.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CD3D11TextureLoaderDDS, 'DDS1', Resources::CTextureLoader);

const Core::CRTTI& CD3D11TextureLoaderDDS::GetResultType() const
{
	return Render::CD3D11Texture::RTTI;
}
//---------------------------------------------------------------------

bool CD3D11TextureLoaderDDS::Load(CResource& Resource)
{
	if (GPU.IsNullPtr()) FAIL;

	const char* pURI = Resource.GetUID().CStr();
	IO::PStream File = IOSrv->CreateStream(pURI);
	if (!File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
	IO::CBinaryReader Reader(*File);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 0x20534444) FAIL; // 'DDS '

	Render::PD3D11Texture Texture = n_new(Render::CD3D11Texture);

	Resource.Init(Texture.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}