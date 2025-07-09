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

static void DFSCoundNodesAndDepth(const CBTNodeInfo& CurrNodeData, U16 Depth, U16& NodeCount, U16& MaxDepth)
{
	++NodeCount;
	if (MaxDepth < Depth) MaxDepth = Depth;

	for (const auto& ChildNodeData : CurrNodeData.Children)
		DFSCoundNodesAndDepth(ChildNodeData, Depth + 1, NodeCount, MaxDepth);
}
//---------------------------------------------------------------------

static bool DFSUnwrapAndPrepareNodes(CBTNodeInfo& CurrNodeData, CBTNodeInfo* pNodeData, U16& CurrIdx, U16 ParentIndex, U16 DepthLevel)
{
	//???store all basic nodes in one header? implementations can be very simple and defined right in headers
	//???register basic nodes with easy to read IDs like basic conditions are registered in RegisterCondition? First try them, then the factory.
	// TODO: (de)serialization for RTTI? From string or int hash/FourCC.
	CurrNodeData.pRTTI = DEM::Core::CFactory::Instance().GetRTTI(CurrNodeData.ClassName.CStr()); // TODO: use CStrID in factory
	if (!CurrNodeData.pRTTI || !CurrNodeData.pRTTI->IsDerivedFrom(DEM::AI::CBehaviourTreeNodeBase::RTTI))
	{
		::Sys::Error("Behaviour tree node class is not found or is not a subclass of CBehaviourTreeNodeBase");
		return false;
	}

	// To preserve DFS indices after sorting node info array
	CurrNodeData.Index = CurrIdx;

	for (auto& ChildNodeData : CurrNodeData.Children)
	{
		pNodeData[++CurrIdx] = std::move(ChildNodeData);
		if (!DFSUnwrapAndPrepareNodes(pNodeData[++CurrIdx], pNodeData, ++CurrIdx, CurrNodeData.Index, DepthLevel + 1))
			return false;
	}

	// Children are DFS-unwrapped
	std::exchange(CurrNodeData.Children, {});

	CurrNodeData.SkipSubtreeIndex = CurrIdx;

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
	// TODO: immediately unwrap with DFS?
	std::vector<CBTNodeInfo> NodeInfo;
	NodeInfo.resize(1);
	DEM::ParamsFormat::Deserialize(pRootDesc->GetRawValue(), NodeInfo[0]);

	U16 NodeCount = 0;
	U16 MaxDepth = 0;
	DFSCoundNodesAndDepth(NodeInfo[0], 1, NodeCount, MaxDepth);

	n_assert_dbg(NodeCount && MaxDepth);
	if (!NodeCount || !MaxDepth) return nullptr;

	NodeInfo.resize(NodeCount);

	U16 CurrIdx = 0;
	if (!DFSUnwrapAndPrepareNodes(NodeInfo[0], NodeInfo.data(), CurrIdx, 0, 0)) return nullptr;

	// Sort nodes by alignment and size of static data for optimal packing into a single buffer (see below)
	std::sort(NodeInfo.begin(), NodeInfo.end(), [](const auto& a, const auto& b)
	{
		if (a.pRTTI->GetInstanceAlignment() != b.pRTTI->GetInstanceAlignment())
			return a.pRTTI->GetInstanceAlignment() > b.pRTTI->GetInstanceAlignment();
		if (a.pRTTI->GetInstanceSize() != b.pRTTI->GetInstanceSize())
			return a.pRTTI->GetInstanceSize() > b.pRTTI->GetInstanceSize();
		return a.Index < b.Index;
	});

	return new DEM::AI::CBehaviourTreeAsset(NodeInfo.data(), NodeCount, MaxDepth);
}
//---------------------------------------------------------------------

}
