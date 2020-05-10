#include "NavAgentSettingsLoaderHRD.h"
#include <AI/Navigation/NavAgentSettings.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/Params.h>

namespace Resources
{

const Core::CRTTI& CNavAgentSettingsLoaderHRD::GetResultType() const
{
	return DEM::AI::CNavAgentSettings::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CNavAgentSettingsLoaderHRD::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID, pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	// Read params from resource HRD
	Data::CParams Params;
	Data::CHRDParser Parser;
	if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), Params)) return nullptr;

	// Load settings
	// ...

	return n_new(DEM::AI::CNavAgentSettings());
}
//---------------------------------------------------------------------

}
