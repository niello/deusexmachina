#include "MaterialLoader.h"

#include <Render/GPUDriver.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/EffectLoader.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CMaterialLoader, Resources::CResourceLoader);

const Core::CRTTI& CMaterialLoader::GetResultType() const
{
	return Render::CMaterial::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CMaterialLoader::Load(IO::CStream& Stream)
{
	if (GPU.IsNullPtr()) FAIL;

	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'MTRL') FAIL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) FAIL;

	CStrID EffectID;
	if (!Reader.Read(EffectID)) FAIL;
	if (!EffectID.IsValid()) FAIL;

	CString RsrcURI("Effects:");
	RsrcURI += EffectID.CStr();
	RsrcURI += ".eff"; //???replace ID by full URI on export?

	Resources::PResource Rsrc = ResourceMgr->RegisterResource(RsrcURI.CStr());
	if (!Rsrc->IsLoaded())
	{
		Resources::PResourceLoader Loader = Rsrc->GetLoader();
		if (Loader.IsNullPtr())
		{
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CEffect>(PathUtils::GetExtension(RsrcURI.CStr()));
			if (Loader.IsNullPtr()) FAIL;
		}
		Loader->As<Resources::CEffectLoader>()->GPU = GPU;
		ResourceMgr->LoadResourceSync(*Rsrc, *Loader);
		if (!Rsrc->IsLoaded()) FAIL;
	}

	Render::PMaterial Mtl = n_new(Render::CMaterial);

	Mtl->Effect = Rsrc->GetObject<Render::CEffect>();

	// Build parameters

	CLoadedValues Values;
	if (!LoadEffectParamValues(Reader, GPU, Values)) FAIL;

	const CFixedArray<Render::CEffectConstant>& Consts = Mtl->Effect->GetMaterialConstants();
	Mtl->ConstBuffers.SetSize(Mtl->Effect->GetMaterialConstantBufferCount());
	UPTR CurrCBCount = 0;
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Const = Consts[i];

		Render::CMaterial::CConstBufferRec* pRec = NULL;
		for (UPTR BufIdx = 0; BufIdx < CurrCBCount; ++BufIdx)
			if (Mtl->ConstBuffers[BufIdx].Handle == Const.BufferHandle)
			{
				pRec = &Mtl->ConstBuffers[BufIdx];
				break;
			}
		
		if (!pRec)
		{
			pRec = &Mtl->ConstBuffers[CurrCBCount];
			pRec->Handle = Const.BufferHandle;
			pRec->ShaderType = Const.ShaderType;
			pRec->Buffer = GPU->CreateConstantBuffer(Const.BufferHandle, Render::Access_CPU_Write); //!!!must be a RAM-only buffer!
			++CurrCBCount;

			if (!GPU->BeginShaderConstants(*pRec->Buffer.GetUnsafe())) FAIL;
		}

		IPTR Idx = Values.Consts.FindIndex(Const.ID);
		void* pValue = (Idx != INVALID_INDEX) ? Values.Consts.ValueAt(Idx) : NULL;
		if (!pValue) pValue = Const.pDefaultValue;

		if (pValue) //???fail if value is undefined? or fill with zeroes?
		{
			//???!!!add to SetShaderConstant() zero size support - autocalc?! tool-side validation!
			if (!GPU->SetShaderConstant(*pRec->Buffer.GetUnsafe(), Const.Handle, 0, pValue, Const.SizeInBytes)) FAIL;
		}
	}

	for (UPTR BufIdx = 0; BufIdx < CurrCBCount; ++BufIdx)
	{
		Render::CMaterial::CConstBufferRec* pRec = &Mtl->ConstBuffers[BufIdx];
		Render::PConstantBuffer RAMBuffer = pRec->Buffer;
		if (!GPU->CommitShaderConstants(*RAMBuffer.GetUnsafe())) FAIL; //!!!must not do any VRAM operations inside!

		//???do only if current buffer doesn't support VRAM? DX9 will support, DX11 will not.
		//if supports VRAM, can reuse as VRAM buffer without data copying between RAMBuffer and a new one.
		pRec->Buffer = GPU->CreateConstantBuffer(pRec->Handle, Render::Access_GPU_Read, RAMBuffer.GetUnsafe());
	}

	const CFixedArray<Render::CEffectResource>& Resources = Mtl->Effect->GetMaterialResources();
	Mtl->Resources.SetSize(Resources.GetCount());
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		const Render::CEffectResource& Rsrc = Resources[i];
		
		IPTR Idx = Values.Resources.FindIndex(Rsrc.ID);
		Render::PTexture Value = (Idx != INVALID_INDEX) ? Values.Resources.ValueAt(Idx) : Render::PTexture();
		
		Render::CMaterial::CResourceRec& Rec = Mtl->Resources[i];
		Rec.Handle = Rsrc.Handle;
		Rec.ShaderType = Rsrc.ShaderType;
		Rec.Resource = Value.IsValidPtr() ? Value : Rsrc.DefaultValue;
	}

	const CFixedArray<Render::CEffectSampler>& Samplers = Mtl->Effect->GetMaterialSamplers();
	Mtl->Samplers.SetSize(Samplers.GetCount());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		const Render::CEffectSampler& Sampler = Samplers[i];
		
		IPTR Idx = Values.Samplers.FindIndex(Sampler.ID);
		Render::PSampler Value = (Idx != INVALID_INDEX) ? Values.Samplers.ValueAt(Idx) : Render::PSampler();
		
		Render::CMaterial::CSamplerRec& Rec = Mtl->Samplers[i];
		Rec.Handle = Sampler.Handle;
		Rec.ShaderType = Sampler.ShaderType;
		Rec.Sampler = Value.IsValidPtr() ? Value : Sampler.DefaultValue;
	}

	return Mtl.GetUnsafe();
}
//---------------------------------------------------------------------

bool CMaterialLoader::LoadEffectParamValues(IO::CBinaryReader& Reader, Render::PGPUDriver GPU, CLoadedValues& OutValues)
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
				
				if (!OutValues.Consts.IsInAddMode()) OutValues.Consts.BeginAdd();
				OutValues.Consts.Add(ParamID, (void*)Offset);

				break;
			}
			case Render::EPT_Resource:
			{
				Render::PTexture Texture = CEffectLoader::LoadTextureValue(Reader, GPU);
				if (Texture.IsNullPtr()) FAIL;

				if (!OutValues.Resources.IsInAddMode()) OutValues.Resources.BeginAdd();
				OutValues.Resources.Add(ParamID, Texture);

				break;
			}
			case Render::EPT_Sampler:
			{
				Render::PSampler Sampler = CEffectLoader::LoadSamplerValue(Reader, GPU);
				if (Sampler.IsNullPtr()) FAIL;
				
				if (!OutValues.Samplers.IsInAddMode()) OutValues.Samplers.BeginAdd();
				OutValues.Samplers.Add(ParamID, Sampler);
						
				break;
			}
		}
	}

	if (OutValues.Consts.IsInAddMode()) OutValues.Consts.EndAdd();
	if (OutValues.Resources.IsInAddMode()) OutValues.Resources.EndAdd();
	if (OutValues.Samplers.IsInAddMode()) OutValues.Samplers.EndAdd();

	U32 DefValsSize;
	if (!Reader.Read(DefValsSize)) FAIL;
	if (DefValsSize)
	{
		OutValues.pConstValueBuffer = n_malloc(DefValsSize);
		Reader.GetStream().Read(OutValues.pConstValueBuffer, DefValsSize);

		for (UPTR i = 0; i < OutValues.Consts.GetCount(); ++i)
		{
			void*& pValue = OutValues.Consts.ValueAt(i);
			pValue = (char*)OutValues.pConstValueBuffer + (UPTR)pValue;
		}
	}

	OK;
}
//---------------------------------------------------------------------

}