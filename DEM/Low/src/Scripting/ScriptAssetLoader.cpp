#include "ScriptAssetLoader.h"
#include <Scripting/ScriptAsset.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>

namespace Resources
{

const Core::CRTTI& CScriptAssetLoader::GetResultType() const
{
	return DEM::Scripting::CScriptAsset::RTTI;
}
//---------------------------------------------------------------------

Core::PObject CScriptAssetLoader::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	return n_new(DEM::Scripting::CScriptAsset(std::string(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize())));
}
//---------------------------------------------------------------------

}
