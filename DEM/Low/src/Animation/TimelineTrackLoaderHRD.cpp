#include "TimelineTrackLoaderHRD.h"
#include <Animation/TimelineTrack.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Params.h>
#include <Data/Buffer.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>

namespace Resources
{

const Core::CRTTI& CTimelineTrackLoaderHRD::GetResultType() const
{
	return DEM::Anim::CTimelineTrack::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CTimelineTrackLoaderHRD::CreateResource(CStrID UID)
{
	// TODO: can support optional multiple templates in one file through sub-ID
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

	NOT_IMPLEMENTED;
	return nullptr;
}
//---------------------------------------------------------------------

}
