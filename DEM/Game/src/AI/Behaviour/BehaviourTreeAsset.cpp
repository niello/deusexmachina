#include "BehaviourTreeAsset.h"
#include <Math/Math.h>
#include <Core/RTTI.h>
#include <Core/Factory.h>
#include <cstdlib>

namespace DEM::AI
{

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
	n_assert_dbg(CurrNodeInfo.pRTTI);
	if (!CurrNodeInfo.pRTTI) return false;

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
	n_assert_dbg(NodeCount);

	// Fill a temporary buffer with node information needed to build an asset
	std::unique_ptr<CNodeInfo[]> NodeInfo(new CNodeInfo[NodeCount]);
	size_t CurrIdx = 0;
	DFSSecondPass(RootNodeData, NodeInfo.get(), CurrIdx);

	// Sort nodes by alignment and size of static data for optimal packing into a single buffer (see below)
	std::sort(NodeInfo.get(), NodeInfo.get() + NodeCount, [](const auto& a, const auto& b)
	{
		if (a.pRTTI->GetInstanceAlignment() != b.pRTTI->GetInstanceAlignment())
			return a.pRTTI->GetInstanceAlignment() > b.pRTTI->GetInstanceAlignment();
		if (a.pRTTI->GetInstanceSize() != b.pRTTI->GetInstanceSize())
			return a.pRTTI->GetInstanceSize() > b.pRTTI->GetInstanceSize();
		return a.Index < b.Index;
	});

	// Calculate memory needed for node pointer and skip index array
	const size_t StaticAlignment = std::max(sizeof(void*), NodeInfo[0].pRTTI->GetInstanceAlignment());
	size_t StaticBytes = 0;

	// Add shared node data memory requirements
	for (size_t i = 0; i < NodeCount; ++i)
		StaticBytes += NodeInfo[i].pRTTI->GetInstanceSize();

	// Allocate a single buffer for all asset data
	unique_ptr_aligned<void> StaticBuffer(n_malloc_aligned(StaticBytes, StaticAlignment));
	// std::aligned_alloc / std::free, operator new(std::align_val_t)
	// std::aligned_storage, alignas(Align) unsigned char data[Len];
	// assert that memory is allocated!
	//!!!could sort by alignment!
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
