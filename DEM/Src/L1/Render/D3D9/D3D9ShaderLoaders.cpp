#include "D3D9ShaderLoaders.h"

#include <Resources/Resource.h>
#include <Render/D3D9/D3D9GPUDriver.h>
#include <Render/D3D9/D3D9Shader.h>
#include <IO/IOServer.h>
#include <IO/BinaryReader.h>
#include <IO/Streams/FileStream.h>
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

bool CD3D9ShaderLoader::LoadImpl(CResource& Resource, Render::EShaderType ShaderType)
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D9GPUDriver>()) FAIL;

	//!!!open stream from URI w/optional IO cache! not necessarily a file stream!
	//!!!some streams don't support Seek and GetSize!
	IO::CFileStream IOStream(Resource.GetUID().CStr());
	if (!IOStream.Open(IO::SAM_READ, IO::SAP_RANDOM)) FAIL;
	DWORD FileSize = IOStream.GetSize();

	IO::CBinaryReader R(IOStream);

	Data::CFourCC FileSig;
	if (!R.Read(FileSig)) FAIL;

	switch (ShaderType)
	{
		case Render::ShaderType_Vertex:		if (FileSig != 'VS30') FAIL; break;
		case Render::ShaderType_Pixel:		if (FileSig != 'PS30') FAIL; break;
		default:
		{
			// Shader type autodetection
			switch (FileSig.Code)
			{
				case 'VS30':	ShaderType = Render::ShaderType_Vertex; break;
				case 'PS30':	ShaderType = Render::ShaderType_Pixel; break;
				default:		FAIL;
			};
			break;
		}
	}

	U32 BinaryOffset;
	if (!R.Read(BinaryOffset)) FAIL;

	U32 ShaderFileID;
	if (!R.Read(ShaderFileID)) FAIL;

	//???U64?
	UPTR MetadataOffset = IOStream.GetPosition();

	UPTR BinarySize = FileSize - BinaryOffset;
	if (!BinarySize) FAIL;
	void* pData = n_malloc(BinarySize);
	if (!pData) FAIL;
	if (!IOStream.Seek(BinaryOffset, IO::Seek_Begin) || IOStream.Read(pData, BinarySize) != BinarySize)
	{
		n_free(pData);
		FAIL;
	}

	Render::PD3D9Shader Shader = (Render::CD3D9Shader*)GPU->CreateShader(ShaderType, pData, BinarySize).GetUnsafe();
	if (Shader.IsNullPtr()) FAIL;

	n_free(pData);

	if (!IOStream.Seek(MetadataOffset, IO::Seek_Begin)) FAIL;

	//???really need multiple buffers?
	CFixedArray<Render::CD3D9Shader::CBufferMeta>& Buffers = Shader->Buffers;
	Buffers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		Render::CD3D9Shader::CBufferMeta* pMeta = &Buffers[i];
		if (!R.Read(pMeta->Name)) return NULL;

		CFixedArray<Render::CD3D9Shader::CRange>& Ranges1 = pMeta->Float4;
		Ranges1.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges1.GetCount(); ++r)
		{
			Render::CD3D9Shader::CRange& Range = Ranges1[r];
			if (!R.Read<U32>(Range.Start)) return NULL;
			if (!R.Read<U32>(Range.Count)) return NULL;
		}

		CFixedArray<Render::CD3D9Shader::CRange>& Ranges2 = pMeta->Int4;
		Ranges2.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges2.GetCount(); ++r)
		{
			Render::CD3D9Shader::CRange& Range = Ranges2[r];
			if (!R.Read<U32>(Range.Start)) return NULL;
			if (!R.Read<U32>(Range.Count)) return NULL;
		}

		CFixedArray<Render::CD3D9Shader::CRange>& Ranges3 = pMeta->Bool;
		Ranges3.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges3.GetCount(); ++r)
		{
			Render::CD3D9Shader::CRange& Range = Ranges3[r];
			if (!R.Read<U32>(Range.Start)) return NULL;
			if (!R.Read<U32>(Range.Count)) return NULL;
		}
	}

	CFixedArray<Render::CD3D9Shader::CConstMeta>& Consts = Shader->Consts;
	Consts.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		U8 RegSet;

		Render::CD3D9Shader::CConstMeta* pMeta = &Consts[i];
		if (!R.Read(pMeta->Name)) return NULL;
		if (!R.Read<U32>(pMeta->BufferIndex)) return NULL;
		if (!R.Read<U8>(RegSet)) return NULL;
		if (!R.Read<U32>(pMeta->Offset)) return NULL;
		if (!R.Read<U32>(pMeta->Size)) return NULL;

		pMeta->RegSet = (Render::CD3D9Shader::ERegisterSet)RegSet;
	}

	/*
			W.Write<U32>(Samplers.GetCount());
			for (int i = 0; i < Samplers.GetCount(); ++i)
			{
				const CD3D9ShaderRsrcMeta& Meta = Samplers[i];
				W.Write(Meta.SamplerName);
				W.Write(Meta.TextureName);
				W.Write(Meta.Register);		//!!!need sampler arrays! need one texture to multiple samplers!

				n_msg(VL_DETAILS, "  Sampler: %s (texture %s), %d slot(s) from %d\n", Meta.SamplerName.CStr(), Meta.TextureName.CStr(), 1, Meta.Register);
			}
	*/

	Resource.Init(Shader.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}