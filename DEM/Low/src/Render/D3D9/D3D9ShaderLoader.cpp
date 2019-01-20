#include "D3D9ShaderLoader.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/D3D9/D3D9GPUDriver.h>
#include <Render/D3D9/D3D9Shader.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const Core::CRTTI& CD3D9ShaderLoader::GetResultType() const
{
	return Render::CD3D9Shader::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CD3D9ShaderLoader::CreateResource(CStrID UID)
{
	if (!GPU || !GPU->IsA<Render::CD3D9GPUDriver>()) return nullptr;

	const char* pSubId;
	IO::PStream Stream = OpenStream(UID, pSubId);
	if (!Stream) return nullptr;

	IO::CBinaryReader R(*Stream);

	Data::CFourCC FileSig;
	if (!R.Read(FileSig)) return nullptr;

	// Shader type is autodetected from the file signature
	Render::EShaderType ShaderType;
	switch (FileSig.Code)
	{
		case 'VS30':	ShaderType = Render::ShaderType_Vertex; break;
		case 'PS30':	ShaderType = Render::ShaderType_Pixel; break;
		default:		return nullptr;
	};

	U32 BinaryOffset;
	if (!R.Read(BinaryOffset)) return nullptr;

	U32 ShaderFileID;
	if (!R.Read(ShaderFileID)) return nullptr;

	U64 MetadataOffset = Stream->GetPosition();
	U64 FileSize = Stream->GetSize();
	UPTR BinarySize = (UPTR)FileSize - (UPTR)BinaryOffset;
	if (!BinarySize) return nullptr;
	void* pData = n_malloc(BinarySize);
	if (!pData) return nullptr;
	if (!Stream->Seek(BinaryOffset, IO::Seek_Begin) || Stream->Read(pData, BinarySize) != BinarySize)
	{
		n_free(pData);
		return nullptr;
	}

	Render::PD3D9Shader Shader = (Render::CD3D9Shader*)GPU->CreateShader(ShaderType, pData, BinarySize).Get();
	if (!Shader) return nullptr;

	n_free(pData);

	if (!Stream->Seek(MetadataOffset, IO::Seek_Begin)) return nullptr;

	if (!Shader->Metadata.Load(*Stream)) return nullptr;

	return Shader.Get();
}
//---------------------------------------------------------------------

}