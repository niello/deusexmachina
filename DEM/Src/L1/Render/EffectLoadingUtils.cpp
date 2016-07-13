#include <Render/Shader.h>
#include <Render/ShaderLibrary.h>
#include <Render/ShaderMetadata.h>
#include <Render/Texture.h>
#include <Render/TextureLoader.h>
#include <Render/SamplerDesc.h>
#include <Render/GPUDriver.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Data/FixedArray.h>

// Utility functions for loading data blocks common to several effect-related formats (EFF, RP, MTL).
// No header file exist, include function declarations into other translation units instead.

namespace Resources
{

// Out array will be sorted by ID as parameters are saved sorted by ID
bool LoadEffectParams(IO::CBinaryReader& Reader, Render::PShaderLibrary ShaderLibrary, const Render::IShaderMetadata* pDefaultShaderMeta, CFixedArray<Render::CEffectConstant>& OutConsts, CFixedArray<Render::CEffectResource>& OutResources, CFixedArray<Render::CEffectSampler>& OutSamplers)
{
	U32 ParamCount;

	if (!Reader.Read<U32>(ParamCount)) FAIL;
	OutConsts.SetSize(ParamCount);
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 ShaderType;
		if (!Reader.Read(ShaderType)) FAIL;

		U32 SourceShaderID;
		if (!Reader.Read(SourceShaderID)) FAIL;

		U8 ConstType;
		if (!Reader.Read(ConstType)) FAIL;

		U32 SizeInBytes;
		if (!Reader.Read(SizeInBytes)) FAIL;

		const Render::IShaderMetadata* pShaderMeta;
		if (SourceShaderID && ShaderLibrary.IsValidPtr())
		{
			// Shader will stay alive in a cache, so metadata will be valid
			Render::PShader ParamShader = ShaderLibrary->GetShaderByID(SourceShaderID);
			pShaderMeta = ParamShader->GetMetadata();
		}
		else pShaderMeta = pDefaultShaderMeta;
		if (!pShaderMeta) FAIL;

		Render::CEffectConstant& Rec = OutConsts[ParamIdx];
		Rec.ID = ParamID;
		Rec.Handle = pShaderMeta->GetConstHandle(ParamID);
		Rec.BufferHandle = pShaderMeta->GetConstBufferHandle(Rec.Handle);
		Rec.ShaderType = (Render::EShaderType)ShaderType;
		Rec.SizeInBytes = SizeInBytes;
		Rec.ElementCount = pShaderMeta->GetConstElementCount(Rec.Handle);
	}

	if (!Reader.Read<U32>(ParamCount)) FAIL;
	OutResources.SetSize(ParamCount);
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 ShaderType;
		if (!Reader.Read(ShaderType)) FAIL;

		U32 SourceShaderID;
		if (!Reader.Read(SourceShaderID)) FAIL;

		const Render::IShaderMetadata* pShaderMeta;
		if (SourceShaderID && ShaderLibrary.IsValidPtr())
		{
			// Shader will stay alive in a cache, so metadata will be valid
			Render::PShader ParamShader = ShaderLibrary->GetShaderByID(SourceShaderID);
			pShaderMeta = ParamShader->GetMetadata();
		}
		else pShaderMeta = pDefaultShaderMeta;
		if (!pShaderMeta) FAIL;

		Render::CEffectResource& Rec = OutResources[ParamIdx];
		Rec.ID = ParamID;
		Rec.Handle = pShaderMeta->GetResourceHandle(ParamID);
		Rec.ShaderType = (Render::EShaderType)ShaderType;
	}

	if (!Reader.Read<U32>(ParamCount)) FAIL;
	OutSamplers.SetSize(ParamCount);
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 ShaderType;
		if (!Reader.Read(ShaderType)) FAIL;

		U32 SourceShaderID;
		if (!Reader.Read(SourceShaderID)) FAIL;

		const Render::IShaderMetadata* pShaderMeta;
		if (SourceShaderID && ShaderLibrary.IsValidPtr())
		{
			// Shader will stay alive in a cache, so metadata will be valid
			Render::PShader ParamShader = ShaderLibrary->GetShaderByID(SourceShaderID);
			pShaderMeta = ParamShader->GetMetadata();
		}
		else pShaderMeta = pDefaultShaderMeta;
		if (!pShaderMeta) FAIL;

		Render::CEffectSampler& Rec = OutSamplers[ParamIdx];
		Rec.ID = ParamID;
		Rec.Handle = pShaderMeta->GetSamplerHandle(ParamID);
		Rec.ShaderType = (Render::EShaderType)ShaderType;
	}

	OK;
}
//---------------------------------------------------------------------

Render::PTexture LoadTextureValue(IO::CBinaryReader& Reader, Render::PGPUDriver GPU)
{
	CStrID ResourceID;
	if (!Reader.Read(ResourceID)) return NULL;

	Resources::PResource RTexture = ResourceMgr->RegisterResource(ResourceID.CStr());
	if (!RTexture->IsLoaded())
	{
		Resources::PResourceLoader Loader = RTexture->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CTexture>(PathUtils::GetExtension(ResourceID.CStr()));
		if (Loader.IsNullPtr()) return NULL;
		Loader->As<Resources::CTextureLoader>()->GPU = GPU;
		ResourceMgr->LoadResourceSync(*RTexture, *Loader);
		if (!RTexture->IsLoaded()) return NULL;
	}

	return RTexture->GetObject<Render::CTexture>();
}
//---------------------------------------------------------------------

Render::PSampler LoadSamplerValue(IO::CBinaryReader& Reader, Render::PGPUDriver GPU)
{
	Render::CSamplerDesc SamplerDesc;

	U8 U8Value;
	Reader.Read<U8>(U8Value);
	SamplerDesc.AddressU = (Render::ETexAddressMode)U8Value;
	Reader.Read<U8>(U8Value);
	SamplerDesc.AddressV = (Render::ETexAddressMode)U8Value;
	Reader.Read<U8>(U8Value);
	SamplerDesc.AddressW = (Render::ETexAddressMode)U8Value;
	Reader.Read<U8>(U8Value);
	SamplerDesc.Filter = (Render::ETexFilter)U8Value;

	Reader.Read(SamplerDesc.BorderColorRGBA[0]);
	Reader.Read(SamplerDesc.BorderColorRGBA[1]);
	Reader.Read(SamplerDesc.BorderColorRGBA[2]);
	Reader.Read(SamplerDesc.BorderColorRGBA[3]);
	Reader.Read(SamplerDesc.MipMapLODBias);
	Reader.Read(SamplerDesc.FinestMipMapLOD);
	Reader.Read(SamplerDesc.CoarsestMipMapLOD);
	Reader.Read(SamplerDesc.MaxAnisotropy);
						
	Reader.Read<U8>(U8Value);
	SamplerDesc.CmpFunc = (Render::ECmpFunc)U8Value;

	return GPU->CreateSampler(SamplerDesc);
}
//---------------------------------------------------------------------

bool LoadEffectParamValues(IO::CBinaryReader& Reader, Render::PGPUDriver GPU, CDict<CStrID, void*>& OutConsts, CDict<CStrID, Render::PTexture>& OutResources, CDict<CStrID, Render::PSampler>& OutSamplers, void*& pOutConstValueBuffer)
{
	U32 ParamCount;
	if (!Reader.Read<U32>(ParamCount)) FAIL;
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 Type;
		if (!Reader.Read(Type)) FAIL;

		switch (Type)
		{
			case Render::EPT_Const:
			{
				U32 Offset;
				if (!Reader.Read(Offset)) FAIL;
				if (!OutConsts.IsInAddMode()) OutConsts.BeginAdd();
				OutConsts.Add(ParamID, (void*)Offset);
				break;
			}
			case Render::EPT_Resource:
			{
				Render::PTexture Texture = LoadTextureValue(Reader, GPU);
				if (Texture.IsNullPtr()) FAIL;
				if (!OutResources.IsInAddMode()) OutResources.BeginAdd();
				OutResources.Add(ParamID, Texture);
				break;
			}
			case Render::EPT_Sampler:
			{
				Render::PSampler Sampler = LoadSamplerValue(Reader, GPU);
				if (Sampler.IsNullPtr()) FAIL;
				if (!OutSamplers.IsInAddMode()) OutSamplers.BeginAdd();
				OutSamplers.Add(ParamID, Sampler);
				break;
			}
		}
	}

	if (OutConsts.IsInAddMode()) OutConsts.EndAdd();
	if (OutResources.IsInAddMode()) OutResources.EndAdd();
	if (OutSamplers.IsInAddMode()) OutSamplers.EndAdd();

	U32 ValueBufferSize;
	if (!Reader.Read(ValueBufferSize)) FAIL;
	if (ValueBufferSize)
	{
		pOutConstValueBuffer = n_malloc(ValueBufferSize);
		Reader.GetStream().Read(pOutConstValueBuffer, ValueBufferSize);
		//???return ValueBufferSize too?

		for (UPTR i = 0; i < OutConsts.GetCount(); ++i)
		{
			void*& pValue = OutConsts.ValueAt(i);
			pValue = (char*)pOutConstValueBuffer + (U32)pValue;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool SkipEffectParams(IO::CBinaryReader& Reader)
{
	// Constants
	U32 Count;
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(10, IO::Seek_Current)) FAIL;
	}

	// Resources
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(5, IO::Seek_Current)) FAIL;
	}

	// Samplers
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(5, IO::Seek_Current)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

}