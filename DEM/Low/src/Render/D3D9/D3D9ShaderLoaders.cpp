#include "D3D9ShaderLoaders.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/D3D9/D3D9GPUDriver.h>
#include <Render/D3D9/D3D9Shader.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CD3D9ShaderLoader, 'SHL9', Resources::CShaderLoader);

///////////////////////////////////////////////////////////////////////
// CD3D9ShaderLoader
///////////////////////////////////////////////////////////////////////

const Core::CRTTI& CD3D9ShaderLoader::GetResultType() const
{
	return Render::CD3D9Shader::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CD3D9ShaderLoader::LoadImpl(IO::CStream& Stream, Render::EShaderType ShaderType)
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D9GPUDriver>()) return NULL;

	IO::CBinaryReader R(Stream);

	Data::CFourCC FileSig;
	if (!R.Read(FileSig)) return NULL;

	switch (ShaderType)
	{
		case Render::ShaderType_Vertex:	if (FileSig != 'VS30') return NULL; break;
		case Render::ShaderType_Pixel:	if (FileSig != 'PS30') return NULL; break;
		default:
		{
			// Shader type autodetection
			switch (FileSig.Code)
			{
				case 'VS30':	ShaderType = Render::ShaderType_Vertex; break;
				case 'PS30':	ShaderType = Render::ShaderType_Pixel; break;
				default:		return NULL;
			};
			break;
		}
	}

	U32 BinaryOffset;
	if (!R.Read(BinaryOffset)) return NULL;

	U32 ShaderFileID;
	if (!R.Read(ShaderFileID)) return NULL;

	U64 MetadataOffset = Stream.GetPosition();
	U64 FileSize = Stream.GetSize();
	UPTR BinarySize = (UPTR)FileSize - (UPTR)BinaryOffset;
	if (!BinarySize) return NULL;
	void* pData = n_malloc(BinarySize);
	if (!pData) return NULL;
	if (!Stream.Seek(BinaryOffset, IO::Seek_Begin) || Stream.Read(pData, BinarySize) != BinarySize)
	{
		n_free(pData);
		return NULL;
	}

	Render::PD3D9Shader Shader = (Render::CD3D9Shader*)GPU->CreateShader(ShaderType, pData, BinarySize).Get();
	if (Shader.IsNullPtr()) return NULL;

	n_free(pData);

	if (!Stream.Seek(MetadataOffset, IO::Seek_Begin)) return NULL;

	if (!Shader->Metadata.Load(Stream)) return NULL;

	return Shader.Get();
}
//---------------------------------------------------------------------

}