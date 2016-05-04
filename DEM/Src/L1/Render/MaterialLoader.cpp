#include "MaterialLoader.h"

#include <Render/GPUDriver.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/EffectLoader.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
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

	CStrID EffectID = Desc->Get<CStrID>(CStrID("Effect"), CStrID::Empty);
	if (!EffectID.IsValid()) FAIL;

	//CString RsrcURI("Shaders:");
	//RsrcURI += EffectID.CStr();
	CString RsrcURI(EffectID.CStr());

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
	//const CFixedArray<CEffectConstant>&	GetMaterialConstants() const { return MaterialConsts; }
	//const CFixedArray<CEffectResource>&	GetMaterialResources() const { return MaterialResources; }
	//const CFixedArray<CEffectSampler>&	GetMaterialSamplers() const { return MaterialSamplers; }

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

		void* pValue = NULL;
		UPTR ValueSize = 0;
		// try to find value in a material description
		if (!pValue)
		{
			pValue = Const.pDefaultValue;
			//!!!size!
		}

		if (!GPU->SetShaderConstant(*pRec->Buffer.GetUnsafe(), Const.Handle, 0, pValue, ValueSize)) FAIL;
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
		
		Render::PTexture Value;
		// try to find value in a material description
		if (Value.IsNullPtr()) Value = Rsrc.DefaultValue;
		
		Render::CMaterial::CResourceRec& Rec = Mtl->Resources[i];
		Rec.Handle = Rsrc.Handle;
		Rec.ShaderType = Rsrc.ShaderType;
		Rec.Resource = Value;
	}

	const CFixedArray<Render::CEffectSampler>& Samplers = Mtl->Effect->GetMaterialSamplers();
	Mtl->Samplers.SetSize(Samplers.GetCount());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		const Render::CEffectSampler& Sampler = Samplers[i];
		
		Render::PSampler Value;
		// try to find value in a material description
		if (Value.IsNullPtr()) Value = Sampler.DefaultValue;
		
		Render::CMaterial::CSamplerRec& Rec = Mtl->Samplers[i];
		Rec.Handle = Sampler.Handle;
		Rec.ShaderType = Sampler.ShaderType;
		Rec.Sampler = Value;
	}

	Resource.Init(Mtl.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}