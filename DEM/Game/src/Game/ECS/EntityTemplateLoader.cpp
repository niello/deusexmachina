#include "EntityTemplateLoader.h"
#include <Game/ECS/EntityTemplate.h>

namespace Resources
{

const Core::CRTTI& CEntityTemplateLoader::GetResultType() const
{
	return DEM::Game::CEntityTemplate::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CEntityTemplateLoader::CreateResource(CStrID UID)
{
	if (!pResMgr) return nullptr;

	//const char* pOutSubId;
	//IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId, IO::SAP_SEQUENTIAL);
	//if (!Stream || !Stream->IsOpened()) return nullptr;

	/*
	IO::PStream File = IOSrv->CreateStream(pFileName, IO::SAM_READ, IO::SAP_SEQUENTIAL);
	if (!File || !File->IsOpened()) return nullptr;
	auto Buffer = File->ReadAll();
	if (!Buffer) return nullptr;

	Data::PParams Params;
	Data::CHRDParser Parser;
	//CString Errors;
	return Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), Params, &Errors) ?
		Params :
		nullptr;

/////////////////////////////////

	Data::PParams Main = ParamsUtils::LoadParamsFromPRM(CString(pRootPath) + pRelativeFileName);
	if (!Main) return nullptr;

	CString BaseName;
	if (!Main->TryGet(BaseName, CStrID("_Base_"))) return Main;

	if (BaseName == pRelativeFileName)
	{
		::Sys::Error("LoadDescFromPRM() > _Base_ can't be self!");
		return nullptr;
	}

	Data::PParams Params = LoadDescFromPRM(pRootPath, BaseName + ".prm");
	if (!Params) return nullptr;

	Params->Merge(*Main, Data::Merge_AddNew | Data::Merge_Replace | Data::Merge_Deep); //!!!can specify merge flags in Desc!

	return Params;
	
	*/

	return nullptr;
}
//---------------------------------------------------------------------

}
