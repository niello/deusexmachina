#include "BehaviourTreeAssetLoader.h"
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/SerializeToParams.h>

namespace Resources
{

const DEM::Core::CRTTI& CBehaviourTreeAssetLoader::GetResultType() const
{
	return DEM::AI::CBehaviourTreeAsset::RTTI;
}
//---------------------------------------------------------------------

//???TODO: can replace this loader with CDataAssetLoaderHRD<CBehaviourTreeAsset>? Must sort actions! There is no constructor with args!
DEM::Core::PObject CBehaviourTreeAssetLoader::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	// Read params from resource HRD
	// FIXME: add an ability to create CData from CParams instead of PParams?
	//Data::CParams Params;
	Data::PParams Params(n_new(Data::CParams)());
	Data::CHRDParser Parser;
	if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), *Params)) return nullptr;

	auto* pRootDesc = Params->Find(CStrID("Root"));
	if (!pRootDesc) return nullptr;

	DEM::AI::CBehaviourTreeNodeData RootNodeData;
	DEM::ParamsFormat::Deserialize(pRootDesc->GetRawValue(), RootNodeData);

	return new DEM::AI::CBehaviourTreeAsset(std::move(RootNodeData));
}
//---------------------------------------------------------------------

}
