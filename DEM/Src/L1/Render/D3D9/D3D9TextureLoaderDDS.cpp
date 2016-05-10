#include "D3D9TextureLoaderDDS.h"

#include <Render/D3D9/D3D9Texture.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CD3D9TextureLoaderDDS, 'DDS9', Resources::CTextureLoader);

const Core::CRTTI& CD3D9TextureLoaderDDS::GetResultType() const
{
	return Render::CD3D9Texture::RTTI;
}
//---------------------------------------------------------------------

bool CD3D9TextureLoaderDDS::Load(CResource& Resource)
{
	if (GPU.IsNullPtr()) FAIL;

	const char* pURI = Resource.GetUID().CStr();
	IO::PStream File = IOSrv->CreateStream(pURI);
	if (!File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
	IO::CBinaryReader Reader(*File);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'DDS ') FAIL;

	Render::PD3D9Texture Texture = n_new(Render::CD3D9Texture);

	Resource.Init(Texture.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}