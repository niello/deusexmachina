#include "FlowAssetLoader.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/SerializeToParams.h>

namespace Resources
{

const Core::CRTTI& CFlowAssetLoader::GetResultType() const
{
	return DEM::Flow::CFlowAsset::RTTI;
}
//---------------------------------------------------------------------

//???TODO: can replace this loader with CDataAssetLoaderHRD<CFlowAsset>? Must sort actions! There is no constructor with args!
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

	// Read params from resource HRD
	// FIXME: add an ability to create CData from CParams instead of PParams?
	//Data::CParams Params;
	Data::PParams Params(n_new(Data::CParams)());
	Data::CHRDParser Parser;
	if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), *Params)) return nullptr;

	auto* pActionsDesc = Params->Find(CStrID("Actions"));
	if (!pActionsDesc) return nullptr;

	std::vector<DEM::Flow::CFlowActionData> Actions;
	DEM::ParamsFormat::Deserialize(pActionsDesc->GetRawValue(), Actions);

	const auto StartActionID = static_cast<U32>(Params->Get<int>(CStrID("StartActionID"), DEM::Flow::EmptyActionID));

	DEM::Flow::CFlowVarStorage Vars;
	if (auto* pVarsDesc = Params->Find(CStrID("Vars")))
		DEM::ParamsFormat::Deserialize(pVarsDesc->GetRawValue(), Vars);

	return n_new(DEM::Flow::CFlowAsset(std::move(Actions), std::move(Vars), StartActionID));
}
//---------------------------------------------------------------------

}
