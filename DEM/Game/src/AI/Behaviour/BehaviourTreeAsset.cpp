#include "BehaviourTreeAsset.h"
#include <Math/Math.h>
#include <Core/RTTI.h>
#include <Core/Factory.h>
#include <cstdlib>

namespace DEM::AI
{

struct CNodeInfo
{
	const DEM::Core::CRTTI* pRTTI;
	size_t             SkipSubtreeIndex;
};

static void DFSFirstPass(const CBehaviourTreeNodeData& NodeData, size_t Depth, size_t& NodeCount, size_t& MaxDepth)
{
	++NodeCount;
	if (MaxDepth < Depth) MaxDepth = Depth;

	for (const auto& ChildNodeData : NodeData.Children)
		DFSFirstPass(ChildNodeData, Depth + 1, NodeCount, MaxDepth);
}
//---------------------------------------------------------------------

static void DFSSecondPass(const CBehaviourTreeNodeData& NodeData, CNodeInfo* pNodeInfo, size_t& CurrIdx)
{
	auto& CurrNodeInfo = pNodeInfo[CurrIdx];
	CurrNodeInfo.pRTTI = DEM::Core::CFactory::Instance().GetRTTI(NodeData.ClassName.CStr()); // TODO: use CStrID in factory
	n_assert_dbg(CurrNodeInfo.pRTTI);

	for (const auto& ChildNodeData : NodeData.Children)
		DFSSecondPass(ChildNodeData, pNodeInfo, ++CurrIdx);

	CurrNodeInfo.SkipSubtreeIndex = CurrIdx;
}
//---------------------------------------------------------------------

// TODO: most of this can be calculated offline
CBehaviourTreeAsset::CBehaviourTreeAsset(CBehaviourTreeNodeData&& RootNodeData)
{
	// Calculate node count and max depth of the tree
	size_t NodeCount = 0;
	size_t MaxDepth = 0;
	DFSFirstPass(RootNodeData, 1, NodeCount, MaxDepth);

	// Fill a preallocated buffer with node RTTI and subtree skip indices (next sibling, subtree end)
	std::unique_ptr<CNodeInfo[]> NodeInfo(new CNodeInfo[NodeCount]);
	size_t CurrIdx = 0;
	DFSSecondPass(RootNodeData, NodeInfo.get(), CurrIdx);

	// Calculate shared node data memory requirements
	size_t StaticAlignment = sizeof(void*);
	size_t StaticBytes = 0;
	for (size_t i = 0; i < NodeCount; ++i)
	{
		auto& CurrNodeInfo = NodeInfo[i];
		if (auto* pRTTI = CurrNodeInfo.pRTTI)
		{
			StaticBytes += pRTTI->GetInstanceSize();
			if (StaticAlignment < pRTTI->GetInstanceAlignment())
				StaticAlignment = pRTTI->GetInstanceAlignment();
		}
	}

	StaticBytes = Math::CeilToMultiple(StaticBytes, StaticAlignment);

	unique_ptr_aligned<void> StaticBuffer(n_malloc_aligned(StaticBytes, StaticAlignment));
	// std::aligned_alloc / std::free, operator new(std::align_val_t)
	// std::aligned_storage, alignas(Align) unsigned char data[Len];
	// assert that memory is allocated!
	//
	//auto storage = static_cast<Foo*>(std::aligned_alloc(n * sizeof(Foo), alignment));
	//std::uninitialized_default_construct_n(storage, n);
	//auto ptr = std::launder(storage);
	//// use ptr to refer to Foo objects
	//std::destroy_n(storage, n);
	//free(storage);

	// Calculate per-instance node data memory requirements. This will be used by BT players.
	size_t InstanceAlignment = sizeof(void*);
	size_t InstanceBytes = 0;
	//...
	InstanceBytes = Math::CeilToMultiple(InstanceBytes, InstanceAlignment);
}
//---------------------------------------------------------------------

}
