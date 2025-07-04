#include "BehaviourTreeAsset.h"
#include <Math/Math.h>
#include <Core/RTTI.h>
#include <Core/Factory.h>

namespace DEM::AI
{

// Temporary data used when loading an asset
struct CNodeInfo
{
	const Core::CRTTI*            pRTTI;
	const CBehaviourTreeNodeData* pData;
	U16                           Index;
	U16                           SkipSubtreeIndex;
	U16                           ParentIndex;
	U16                           DepthLevel;
};

static void DFSFirstPass(const CBehaviourTreeNodeData& NodeData, U16 Depth, U16& NodeCount, U16& MaxDepth)
{
	++NodeCount;
	if (MaxDepth < Depth) MaxDepth = Depth;

	for (const auto& ChildNodeData : NodeData.Children)
		DFSFirstPass(ChildNodeData, Depth + 1, NodeCount, MaxDepth);
}
//---------------------------------------------------------------------

static bool DFSSecondPass(const CBehaviourTreeNodeData& NodeData, CNodeInfo* pNodeInfo, U16& CurrIdx, U16 ParentIndex, U16 DepthLevel)
{
	//???store all basic nodes in one header? implementations can be very simple and defined right in headers
	//???register basic nodes with easy to read IDs like basic conditions are registered in RegisterCondition? First try them, then the factory.
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
		if (!DFSSecondPass(ChildNodeData, pNodeInfo, ++CurrIdx, CurrNodeInfo.Index, DepthLevel + 1))
			return false;

	CurrNodeInfo.SkipSubtreeIndex = CurrIdx;

	return true;
}
//---------------------------------------------------------------------

// TODO: most of this can be calculated offline, except for node sizes and alignment, as they can change
CBehaviourTreeAsset::CBehaviourTreeAsset(CBehaviourTreeNodeData&& RootNodeData)
{
	// Calculate node count and max depth of the tree
	U16 NodeCount = 0;
	DFSFirstPass(RootNodeData, 1, NodeCount, _MaxDepth);

	n_assert_dbg(NodeCount);
	if (!NodeCount) return;

	// Fill a temporary buffer with node information needed to build an asset
	std::unique_ptr<CNodeInfo[]> NodeInfo(new CNodeInfo[NodeCount]);
	U16 CurrIdx = 0;
	if (!DFSSecondPass(RootNodeData, NodeInfo.get(), CurrIdx, 0, 0)) return;

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
	_NodeImplBuffer.reset(static_cast<std::byte*>(n_malloc_aligned(StaticBytes, StaticAlignment)));
	auto* pAddr = _NodeImplBuffer.get();
	for (size_t i = 0; i < NodeCount; ++i)
	{
		auto& CurrNodeInfo = NodeInfo[i];

		auto* pRTTI = CurrNodeInfo.pRTTI;
		auto* pNodeImpl = static_cast<CBehaviourTreeNodeBase*>(pRTTI->CreateInstance(pAddr));
		pNodeImpl->Init(CurrNodeInfo.pData->Params);

		pAddr += pRTTI->GetInstanceSize();

		auto& CurrNode = _Nodes[CurrNodeInfo.Index];
		CurrNode.pNodeImpl = pNodeImpl;
		CurrNode.SkipSubtreeIndex = CurrNodeInfo.SkipSubtreeIndex;
	}

	// Calculate per-instance node data memory requirements. This will be used by BT players.
	// The value may not be exactly an amount of required memory but it is conservative.
	{
		_MaxInstanceBytes = 0;

		constexpr size_t MinInstanceAlignment = sizeof(void*); // 1;?
		std::vector<std::pair<size_t, size_t>> Stack; // Running total byte count, subtree skip index
		size_t CurrIndex = 0;
		Stack.push_back({ 0, NodeCount });
		while (!Stack.empty())
		{
			// First get accumulated size of parents
			size_t TotalSize = Stack.back().first;

			// Now add current node requirements
			const auto& Node = _Nodes[CurrIndex];
			if (const auto DataSize = Node.pNodeImpl->GetInstanceDataSize())
			{
				n_assert_dbg(Node.pNodeImpl->GetInstanceDataAlignment());
				const size_t AlignedSize = Math::CeilToMultiple(DataSize, MinInstanceAlignment);
				const size_t MaxPadding = (Node.pNodeImpl->GetInstanceDataAlignment() / MinInstanceAlignment - 1) * MinInstanceAlignment;
				TotalSize += AlignedSize + MaxPadding;
				if (_MaxInstanceBytes < TotalSize)
					_MaxInstanceBytes = TotalSize;
			}

			++CurrIndex;
			if (Node.SkipSubtreeIndex > CurrIndex)
			{
				// Next node is our child, enter a subtree
				Stack.push_back({ TotalSize, Node.SkipSubtreeIndex });
			}
			else
			{
				// Exit finished subtree
				while (!Stack.empty() && Stack.back().second == CurrIndex)
					Stack.pop_back();
			}
		}
	}
}
//---------------------------------------------------------------------

CBehaviourTreeAsset::~CBehaviourTreeAsset()
{
	const size_t EndIdx = _Nodes[0].SkipSubtreeIndex;
	for (size_t i = 0; i < EndIdx; ++i)
		_Nodes[i].pNodeImpl->~CBehaviourTreeNodeBase();
}
//---------------------------------------------------------------------

}
