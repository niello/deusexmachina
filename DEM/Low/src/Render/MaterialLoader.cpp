#include "MaterialLoader.h"

#include <Render/GPUDriver.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/EffectLoader.h>
#include <Render/ShaderConstant.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CMaterialLoader, Resources::CResourceLoader);

// Defined in Render/EffectLoadingUtils.cpp
bool LoadEffectParamValues(IO::CBinaryReader& Reader, Render::PGPUDriver GPU, CDict<CStrID, void*>& OutConsts, CDict<CStrID, Render::PTexture>& OutResources, CDict<CStrID, Render::PSampler>& OutSamplers, void*& pOutConstValueBuffer);

const Core::CRTTI& CMaterialLoader::GetResultType() const
{
	return Render::CMaterial::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CMaterialLoader::Load(IO::CStream& Stream)
{
	if (GPU.IsNullPtr()) return NULL;

	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'MTRL') return NULL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return NULL;

	CStrID EffectID;
	if (!Reader.Read(EffectID)) return NULL;
	if (!EffectID.IsValid()) return NULL;

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
			if (Loader.IsNullPtr()) return NULL;
		}
		Loader->As<Resources::CEffectLoader>()->GPU = GPU;
		ResourceMgr->LoadResourceSync(*Rsrc, *Loader);
		if (!Rsrc->IsLoaded()) return NULL;
	}

	Render::PMaterial Mtl = n_new(Render::CMaterial);

	Mtl->Effect = Rsrc->GetObject<Render::CEffect>();

	// Build parameters

	CDict<CStrID, void*>			ConstValues;
	CDict<CStrID, Render::PTexture>	ResourceValues;
	CDict<CStrID, Render::PSampler>	SamplerValues;
	void*							pConstValueBuffer;	// Must be n_free()'d if not NULL

	if (!LoadEffectParamValues(Reader, GPU, ConstValues, ResourceValues, SamplerValues, pConstValueBuffer)) return NULL;

	const CFixedArray<Render::CEffectConstant>& Consts = Mtl->Effect->GetMaterialConstants();
	Mtl->ConstBuffers.SetSize(Mtl->Effect->GetMaterialConstantBufferCount());
	UPTR CurrCBCount = 0;
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Const = Consts[i];

		const Render::HConstBuffer hCB = Const.Const->GetConstantBufferHandle();
		Render::CMaterial::CConstBufferRecord* pRec = NULL;
		for (UPTR BufIdx = 0; BufIdx < CurrCBCount; ++BufIdx)
			if (Mtl->ConstBuffers[BufIdx].Handle == hCB)
			{
				pRec = &Mtl->ConstBuffers[BufIdx];
				break;
			}
		
		if (!pRec)
		{
			pRec = &Mtl->ConstBuffers[CurrCBCount];
			pRec->Handle = hCB;
			pRec->ShaderType = Const.ShaderType;
			pRec->Buffer = GPU->CreateConstantBuffer(hCB, Render::Access_CPU_Write); //!!!must be a RAM-only buffer!
			++CurrCBCount;

			if (!GPU->BeginShaderConstants(*pRec->Buffer.GetUnsafe())) return NULL;
		}

		IPTR Idx = ConstValues.FindIndex(Const.ID);
		void* pValue = (Idx != INVALID_INDEX) ? ConstValues.ValueAt(Idx) : NULL;
		if (!pValue) pValue = Mtl->Effect->GetConstantDefaultValue(Const.ID);

		if (pValue) //???fail if value is undefined? or fill with zeroes?
		{
			if (Const.Const.IsNullPtr()) return NULL;
			Const.Const->SetRawValue(*pRec->Buffer.GetUnsafe(), pValue, Render::CShaderConstant::WholeSize);
		}
	}

	SAFE_FREE(pConstValueBuffer);

	for (UPTR BufIdx = 0; BufIdx < CurrCBCount; ++BufIdx)
	{
		Render::CMaterial::CConstBufferRecord* pRec = &Mtl->ConstBuffers[BufIdx];
		Render::PConstantBuffer RAMBuffer = pRec->Buffer;
		if (!GPU->CommitShaderConstants(*RAMBuffer.GetUnsafe())) return NULL; //!!!must not do any VRAM operations inside!

		//???do only if current buffer doesn't support VRAM? DX9 will support, DX11 will not.
		//if supports VRAM, can reuse as VRAM buffer without data copying between RAMBuffer and a new one.
		pRec->Buffer = GPU->CreateConstantBuffer(pRec->Handle, Render::Access_GPU_Read, RAMBuffer.GetUnsafe());
	}

	const CFixedArray<Render::CEffectResource>& Resources = Mtl->Effect->GetMaterialResources();
	Mtl->Resources.SetSize(Resources.GetCount());
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		const Render::CEffectResource& Rsrc = Resources[i];
		
		IPTR Idx = ResourceValues.FindIndex(Rsrc.ID);
		Render::PTexture Value = (Idx != INVALID_INDEX) ? ResourceValues.ValueAt(Idx) : Render::PTexture();
		
		Render::CMaterial::CResourceRecord& Rec = Mtl->Resources[i];
		Rec.Handle = Rsrc.Handle;
		Rec.ShaderType = Rsrc.ShaderType;
		Rec.Resource = Value.IsValidPtr() ? Value : Mtl->Effect->GetResourceDefaultValue(Rsrc.ID);
	}

	const CFixedArray<Render::CEffectSampler>& Samplers = Mtl->Effect->GetMaterialSamplers();
	Mtl->Samplers.SetSize(Samplers.GetCount());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		const Render::CEffectSampler& Sampler = Samplers[i];
		
		IPTR Idx = SamplerValues.FindIndex(Sampler.ID);
		Render::PSampler Value = (Idx != INVALID_INDEX) ? SamplerValues.ValueAt(Idx) : Render::PSampler();
		
		Render::CMaterial::CSamplerRecord& Rec = Mtl->Samplers[i];
		Rec.Handle = Sampler.Handle;
		Rec.ShaderType = Sampler.ShaderType;
		Rec.Sampler = Value.IsValidPtr() ? Value : Mtl->Effect->GetSamplerDefaultValue(Sampler.ID);
	}

	return Mtl.GetUnsafe();
}
//---------------------------------------------------------------------

}