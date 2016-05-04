#include "MaterialLoader.h"

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
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Rsrc = Consts[i];

		// Find buffer instance for a Rsrc.BufferHandle in this material
		// If not found, create CPU/RAM buffer and add record
		Render::PConstantBuffer RAMBuffer;
		Rsrc.ShaderType;
		Rsrc.BufferHandle;

		void* pValue;
		UPTR ValueSize;
		// try to find value in a material description
		// else use default value from an effect, even if NULL

		// Set const value to a buffer
		RAMBuffer;
		Rsrc.Handle;
		pValue;
		ValueSize;
	}

	// for each RAMBuffer created, create GPU/VRAM immutable buffer using CPU/RAM one as an initial data, destroy it then

	const CFixedArray<Render::CEffectResource>& Resources = Mtl->Effect->GetMaterialResources();
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		const Render::CEffectResource& Rsrc = Resources[i];
		Render::PTexture Value;
		// try to find value in a material description
		// else use default value from an effect, even if NULL
		Rsrc.ShaderType;
		Rsrc.Handle;
		Value;
	}

	const CFixedArray<Render::CEffectSampler>& Samplers = Mtl->Effect->GetMaterialSamplers();
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		const Render::CEffectSampler& Sampler = Samplers[i];
		Render::PSampler Value;
		// try to find value in a material description
		// else use default value from an effect, even if NULL
		Sampler.ShaderType;
		Sampler.Handle;
		Value;
	}

	Resource.Init(Mtl.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}