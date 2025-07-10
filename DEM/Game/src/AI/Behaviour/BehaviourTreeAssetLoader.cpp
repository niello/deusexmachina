#include "BehaviourTreeAssetLoader.h"
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/SerializeToParams.h>
#include <Core/Factory.h>

namespace Resources
{
using CBTNodeInfo = DEM::AI::CBehaviourTreeAsset::CNodeInfo;

static bool DFSUnwrapAndPrepareNodes(const Data::CData& CurrNodeData, std::vector<CBTNodeInfo>& NodeInfo, U16 DepthLevel)
{
	auto& CurrNodeInfo = NodeInfo.emplace_back();
	DEM::ParamsFormat::Deserialize(CurrNodeData, CurrNodeInfo);

	//???store all basic nodes in one header? implementations can be very simple and defined right in headers
	//???register basic nodes with easy to read IDs like basic conditions are registered in RegisterCondition? First try them, then the factory.
	// TODO: (de)serialization for RTTI? From string or int hash/FourCC.
	CurrNodeInfo.pRTTI = DEM::Core::CFactory::Instance().GetRTTI(CurrNodeInfo.ClassName.CStr()); // TODO: use CStrID in factory
	if (!CurrNodeInfo.pRTTI || !CurrNodeInfo.pRTTI->IsDerivedFrom(DEM::AI::CBehaviourTreeNodeBase::RTTI))
	{
		::Sys::Error("Behaviour tree node class is not found or is not a subclass of CBehaviourTreeNodeBase");
		return false;
	}

	CurrNodeInfo.DepthLevel = DepthLevel;
	CurrNodeInfo.Index = static_cast<U16>(NodeInfo.size() - 1); // To preserve DFS indices after sorting node info array

	if (auto* pParams = CurrNodeData.As<Data::PParams>())
	{
		Data::PDataArray ChildrenDesc;
		if ((*pParams)->TryGet(ChildrenDesc, CStrID("Children")))
			for (const auto& ChildDesc : *ChildrenDesc)
				if (!DFSUnwrapAndPrepareNodes(ChildDesc, NodeInfo, DepthLevel + 1))
					return false;
	}

	CurrNodeInfo.SkipSubtreeIndex = static_cast<U16>(NodeInfo.size());

	return true;
}
//---------------------------------------------------------------------

const DEM::Core::CRTTI& CBehaviourTreeAssetLoader::GetResultType() const
{
	return DEM::AI::CBehaviourTreeAsset::RTTI;
}
//---------------------------------------------------------------------

// TODO: most of DFS gathered info can be calculated offline
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

	// Parse root, recurse to the hierarchy
	std::vector<CBTNodeInfo> NodeInfo;
	if (!DFSUnwrapAndPrepareNodes(pRootDesc->GetRawValue(), NodeInfo, 0)) return nullptr;

	if (NodeInfo.empty()) return nullptr;

	return new DEM::AI::CBehaviourTreeAsset(std::move(NodeInfo));
}
//---------------------------------------------------------------------

}
