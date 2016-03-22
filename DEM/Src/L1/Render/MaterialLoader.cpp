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
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CEffect>(PathUtils::GetExtension(RsrcURI.CStr()));
		Loader->As<Resources::CEffectLoader>()->GPU = GPU;
		ResourceMgr->LoadResourceSync(*Rsrc, *Loader);
		if (!Rsrc->IsLoaded()) FAIL;
	}

	Render::PMaterial Mtl = n_new(Render::CMaterial);

	// Set effect
	// Build parameters

	Resource.Init(Mtl.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}