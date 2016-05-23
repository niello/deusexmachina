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

	Render::PD3D9Shader Shader = (Render::CD3D9Shader*)GPU->CreateShader(ShaderType, pData, BinarySize).GetUnsafe();
	if (Shader.IsNullPtr()) return NULL;

	n_free(pData);

	if (!Stream.Seek(MetadataOffset, IO::Seek_Begin)) return NULL;

	//???really need multiple buffers in one shader? where to load overridden buffers stored in effect?
	CFixedArray<Render::CD3D9ShaderBufferMeta>& Buffers = Shader->Buffers;
	Buffers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		Render::CD3D9ShaderBufferMeta* pMeta = &Buffers[i];
		if (!R.Read(pMeta->Name)) return NULL;

		CFixedArray<CRange>& Ranges1 = pMeta->Float4;
		Ranges1.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges1.GetCount(); ++r)
		{
			CRange& Range = Ranges1[r];
			if (!R.Read<U32>(Range.Start)) return NULL;
			if (!R.Read<U32>(Range.Count)) return NULL;
		}

		CFixedArray<CRange>& Ranges2 = pMeta->Int4;
		Ranges2.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges2.GetCount(); ++r)
		{
			CRange& Range = Ranges2[r];
			if (!R.Read<U32>(Range.Start)) return NULL;
			if (!R.Read<U32>(Range.Count)) return NULL;
		}

		CFixedArray<CRange>& Ranges3 = pMeta->Bool;
		Ranges3.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges3.GetCount(); ++r)
		{
			CRange& Range = Ranges3[r];
			if (!R.Read<U32>(Range.Start)) return NULL;
			if (!R.Read<U32>(Range.Count)) return NULL;
		}

		pMeta->SlotIndex = i;

		// For non-empty buffers open handles at the load time to reference buffers from constants
		pMeta->Handle =
			(pMeta->Float4.GetCount() || pMeta->Int4.GetCount() || pMeta->Bool.GetCount()) ?
			D3D9DrvFactory->HandleMgr.OpenHandle(pMeta) :
			INVALID_HANDLE;
	}

	CFixedArray<Render::CD3D9ShaderConstMeta>& Consts = Shader->Consts;
	Consts.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		Render::CD3D9ShaderConstMeta* pMeta = &Consts[i];
		if (!R.Read(pMeta->Name)) return NULL;

		U32 BufIdx;
		if (!R.Read<U32>(BufIdx)) return NULL;
		pMeta->BufferHandle = Buffers[BufIdx].Handle;

		U8 RegSet;
		if (!R.Read<U8>(RegSet)) return NULL;
		pMeta->RegSet = (Render::ESM30RegisterSet)RegSet;

		if (!R.Read<U32>(pMeta->RegisterStart)) return NULL;
		if (!R.Read<U32>(pMeta->ElementRegisterCount)) return NULL;
		if (!R.Read<U32>(pMeta->ElementCount)) return NULL;

		pMeta->Handle = INVALID_HANDLE;
	}

	CFixedArray<Render::CD3D9ShaderRsrcMeta>& Resources = Shader->Resources;
	Resources.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		Render::CD3D9ShaderRsrcMeta* pMeta = &Resources[i];
		if (!R.Read(pMeta->Name)) return NULL;
		if (!R.Read<U32>(pMeta->Register)) return NULL;
		
		pMeta->Handle = INVALID_HANDLE;
	}

	CFixedArray<Render::CD3D9ShaderSamplerMeta>& Samplers = Shader->Samplers;
	Samplers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		Render::CD3D9ShaderSamplerMeta* pMeta = &Samplers[i];
		if (!R.Read(pMeta->Name)) return NULL;

		U8 Type;
		if (!R.Read<U8>(Type)) return NULL;
		pMeta->Type = (Render::ESM30SamplerType)Type;
		
		if (!R.Read<U32>(pMeta->RegisterStart)) return NULL;
		if (!R.Read<U32>(pMeta->RegisterCount)) return NULL;

		pMeta->Handle = INVALID_HANDLE;
	}

	return Shader.GetUnsafe();
}
//---------------------------------------------------------------------

}