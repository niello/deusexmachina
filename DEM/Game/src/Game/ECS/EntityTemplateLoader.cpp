#include "EntityTemplateLoader.h"
#include <Game/ECS/EntityTemplate.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>

namespace Resources
{

const Core::CRTTI& CEntityTemplateLoader::GetResultType() const
{
	return DEM::Game::CEntityTemplate::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CEntityTemplateLoader::CreateResource(CStrID UID)
{
	// TODO: can support optional multiple templates in one file through sub-ID
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	// Read params from resource HRD
	Data::CParams Params;
	Data::CHRDParser Parser;
	if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), Params)) return nullptr;

	// Recurse to base files, merge them into main params
	CString BaseName;
	const CStrID sidBase("_Base_");
	while (Params.TryGet(BaseName, sidBase))
	{
		Params.Remove(sidBase);

		{
			// TODO: can support optional multiple templates in one file through sub-ID
			const char* pOutBaseSubId;
			IO::PStream Stream = _ResMgr.CreateResourceStream(BaseName.CStr(), pOutBaseSubId, IO::SAP_SEQUENTIAL);
			if (!Stream || !Stream->IsOpened()) return nullptr;
			Buffer = Stream->ReadAll();
		}

		Data::CParams BaseParams;
		if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), BaseParams)) return nullptr;

		BaseParams.Merge(Params, Data::Merge_AddNew | Data::Merge_Replace | Data::Merge_Deep | Data::Merge_DeleteNulls);
		Params = std::move(BaseParams);
	}

	return n_new(DEM::Game::CEntityTemplate(std::move(Params)));
}
//---------------------------------------------------------------------

}
