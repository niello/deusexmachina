#include "BehaviourTreeAsset.h"
#include <Math/Math.h>
#include <Core/RTTI.h>
#include <Core/Factory.h>
#include <cstdlib>

namespace DEM::AI
{

// Temporary data used when loading an asset
struct CNodeInfo
{
	const Core::CRTTI*            pRTTI;
	const CBehaviourTreeNodeData* pData;
	size_t                        Index;
	size_t                        SkipSubtreeIndex;
};

static void DFSFirstPass(const CBehaviourTreeNodeData& NodeData, size_t Depth, size_t& NodeCount, size_t& MaxDepth)
{
	++NodeCount;
	if (MaxDepth < Depth) MaxDepth = Depth;

	for (const auto& ChildNodeData : NodeData.Children)
		DFSFirstPass(ChildNodeData, Depth + 1, NodeCount, MaxDepth);
}
//---------------------------------------------------------------------

static bool DFSSecondPass(const CBehaviourTreeNodeData& NodeData, CNodeInfo* pNodeInfo, size_t& CurrIdx)
{
	auto& CurrNodeInfo = pNodeInfo[CurrIdx];
	CurrNodeInfo.pRTTI = DEM::Core::CFactory::Instance().GetRTTI(NodeData.ClassName.CStr()); // TODO: use CStrID in factory
	if (!CurrNodeInfo.pRTTI || !CurrNodeInfo.pRTTI->IsDerivedFrom(CBehaviourTreeNodeBase::RTTI))
	{
		::Sys::Error("Behaviour tree node class is not found or is not a subclass of CBehaviourTreeNodeBase");
		return false;
	}

	CurrNodeInfo.pData = &NodeData;

	// To preserve DFS indices after sorting node info array
	CurrNodeInfo.Index = CurrIdx;

	for (const auto& ChildNodeData : NodeData.Children)
		if (!DFSSecondPass(ChildNodeData, pNodeInfo, ++CurrIdx)) return false;

	CurrNodeInfo.SkipSubtreeIndex = CurrIdx;

	return true;
}
//---------------------------------------------------------------------

// TODO: most of this can be calculated offline, except for node sizes and alignment, as they can change
CBehaviourTreeAsset::CBehaviourTreeAsset(CBehaviourTreeNodeData&& RootNodeData)
{
	// Calculate node count and max depth of the tree
	size_t NodeCount = 0;
	size_t MaxDepth = 0;
	DFSFirstPass(RootNodeData, 1, NodeCount, MaxDepth);
	n_assert_dbg(NodeCount > 0);

	// Fill a temporary buffer with node information needed to build an asset
	std::unique_ptr<CNodeInfo[]> NodeInfo(new CNodeInfo[NodeCount]);
	size_t CurrIdx = 0;
	if (!DFSSecondPass(RootNodeData, NodeInfo.get(), CurrIdx)) return;

	// Sort nodes by alignment and size of static data for optimal packing into a single buffer (see below)
	std::sort(NodeInfo.get(), NodeInfo.get() + NodeCount, [](const auto& a, const auto& b)
	{
		if (a.pRTTI->GetInstanceAlignment() != b.pRTTI->GetInstanceAlignment())
			return a.pRTTI->GetInstanceAlignment() > b.pRTTI->GetInstanceAlignment();
		if (a.pRTTI->GetInstanceSize() != b.pRTTI->GetInstanceSize())
			return a.pRTTI->GetInstanceSize() > b.pRTTI->GetInstanceSize();
		return a.Index < b.Index;
	});

	_Nodes.reset(new CNode[NodeCount]);

	// Calculate shared node data memory requirements
	size_t StaticBytes = 0;
	for (size_t i = 0; i < NodeCount; ++i)
		StaticBytes += NodeInfo[i].pRTTI->GetInstanceSize();

	// Allocate a single buffer for node implementations
	const size_t StaticAlignment = std::max(sizeof(void*), NodeInfo[0].pRTTI->GetInstanceAlignment());
	_NodeImplBuffer.reset(n_malloc_aligned(StaticBytes, StaticAlignment));
	auto* pAddr = static_cast<std::byte*>(_NodeImplBuffer.get());
	for (size_t i = 0; i < NodeCount; ++i)
	{
		auto& CurrNodeInfo = NodeInfo[i];

		auto* pRTTI = CurrNodeInfo.pRTTI;
		auto* pNode = static_cast<CBehaviourTreeNodeBase*>(pRTTI->CreateInstance(pAddr));
		pNode->Init(CurrNodeInfo.pData->Params);

		pAddr += pRTTI->GetInstanceSize();

		auto& CurrNode = _Nodes[CurrNodeInfo.Index];
		CurrNode.pNode = pNode;
		CurrNode.SkipSubtreeIndex = CurrNodeInfo.SkipSubtreeIndex;
	}

	// Calculate per-instance node data memory requirements. This will be used by BT players.
	size_t InstanceAlignment = sizeof(void*);
	size_t InstanceBytes = 0;
	//...
	InstanceBytes = Math::CeilToMultiple(InstanceBytes, InstanceAlignment);
}
//---------------------------------------------------------------------

CBehaviourTreeAsset::~CBehaviourTreeAsset()
{
	const size_t EndIdx = _Nodes[0].SkipSubtreeIndex;
	for (size_t i = 0; i < EndIdx; ++i)
		_Nodes[i].pNode->~CBehaviourTreeNodeBase();
}
//---------------------------------------------------------------------

}
