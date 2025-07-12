#include "BehaviourTreeAsset.h"
#include <AI/Command.h>
#include <Math/Math.h>
#include <Core/RTTI.h>

namespace DEM::AI
{

EBTStatus CommandStatusToBTStatus(ECommandStatus Status)
{
	switch (Status)
	{
		case ECommandStatus::Succeeded:
			return EBTStatus::Succeeded;
		case ECommandStatus::Failed:
		case ECommandStatus::Cancelled:
			return EBTStatus::Failed;
		default:
			return EBTStatus::Running;
	}
}
//---------------------------------------------------------------------

CBehaviourTreeAsset::CBehaviourTreeAsset(std::vector<CNodeInfo>&& NodeInfo)
{
	n_assert_dbg(!NodeInfo.empty());

	_MaxDepth = std::max_element(NodeInfo.begin(), NodeInfo.end(), [](const CNodeInfo& a, const CNodeInfo& b) { return a.DepthLevel < b.DepthLevel; })->DepthLevel + 1;

	// Sort nodes by alignment and size of static data for optimal packing into a single buffer (see below)
	std::sort(NodeInfo.begin(), NodeInfo.end(), [](const auto& a, const auto& b)
	{
		if (a.pRTTI->GetInstanceAlignment() != b.pRTTI->GetInstanceAlignment())
			return a.pRTTI->GetInstanceAlignment() > b.pRTTI->GetInstanceAlignment();
		if (a.pRTTI->GetInstanceSize() != b.pRTTI->GetInstanceSize())
			return a.pRTTI->GetInstanceSize() > b.pRTTI->GetInstanceSize();
		return a.Index < b.Index;
	});

	_Nodes.reset(new CNode[NodeInfo.size()]);

	// Calculate shared node data memory requirements
	size_t StaticBytes = 0;
	for (const auto& CurrNodeInfo : NodeInfo)
		StaticBytes += CurrNodeInfo.pRTTI->GetInstanceSize();

	// Allocate a single buffer for node implementations
	const size_t MaxNodeAlignment = NodeInfo[0].pRTTI->GetInstanceAlignment();
	const size_t StaticAlignment = std::max(sizeof(void*), MaxNodeAlignment);
	_NodeImplBuffer.reset(static_cast<std::byte*>(n_malloc_aligned(StaticBytes, StaticAlignment)));
	auto* pAddr = _NodeImplBuffer.get();
	for (const auto& CurrNodeInfo : NodeInfo)
	{
		auto* pRTTI = CurrNodeInfo.pRTTI;
		auto* pNodeImpl = static_cast<CBehaviourTreeNodeBase*>(pRTTI->CreateInstance(pAddr));
		pNodeImpl->Init(CurrNodeInfo.Params.Get());

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
		Stack.push_back({ 0, NodeInfo.size() });
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
