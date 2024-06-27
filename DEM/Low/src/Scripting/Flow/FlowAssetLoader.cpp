#include "FlowAssetLoader.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>

namespace Resources
{

const Core::CRTTI& CFlowAssetLoader::GetResultType() const
{
	return DEM::Flow::CFlowAsset::RTTI;
}
//---------------------------------------------------------------------

Core::PObject CFlowAssetLoader::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	// ...
	//???can use templated data loader? or need some processing?

	return n_new(DEM::Flow::CFlowAsset());
}
//---------------------------------------------------------------------

}
