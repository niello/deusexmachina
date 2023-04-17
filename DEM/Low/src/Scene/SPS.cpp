#include <Scene/SPS.h>
#include <Math/Math.h>

namespace Scene
{

CSPSCell::CIterator CSPSCell::Add(CSPSRecord* Object)
{
	n_assert_dbg(Object && !Object->pPrev);
	if (pFront)
	{
		Object->pNext = pFront;
		pFront->pPrev = Object;
	}
	pFront = Object;
	return CIterator(Object);
}
//---------------------------------------------------------------------

void CSPSCell::Remove(CSPSRecord* pRec)
{
	n_assert_dbg(pRec);
	CSPSRecord* pPrev = pRec->pPrev;
	CSPSRecord* pNext = pRec->pNext;
	if (pPrev) pPrev->pNext = pNext;
	if (pNext) pNext->pPrev = pPrev;
	if (pRec == pFront) pFront = pNext;
	pRec->pNext = nullptr;
	pRec->pPrev = nullptr;
}
//---------------------------------------------------------------------

bool CSPSCell::RemoveByValue(CSPSRecord* Object)
{
	CSPSRecord* pRec = Object;
	CSPSRecord* pCurr = pFront;
	while (pCurr)
	{
		if (pCurr == pRec)
		{
			Remove(pCurr);
			OK;
		}
		pCurr = pCurr->pNext;
	}
	FAIL;
}
//---------------------------------------------------------------------

CSPSCell::CIterator CSPSCell::Find(CSPSRecord* Object) const
{
	CSPSRecord* pRec = Object;
	CSPSRecord* pCurr = pFront;
	while (pCurr)
	{
		if (pCurr == pRec) return CIterator(pCurr);
		pCurr = pCurr->pNext;
	}
	return CIterator(nullptr);
}
//---------------------------------------------------------------------

///////// NEW RENDER /////////
// FIXME: move to math!!!
DEM_FORCE_INLINE U32 MortonCode2(U32 x) noexcept
{
	x &= 0x0000ffff;
	x = (x ^ (x << 8)) & 0x00ff00ff;
	x = (x ^ (x << 4)) & 0x0f0f0f0f;
	x = (x ^ (x << 2)) & 0x33333333;
	x = (x ^ (x << 1)) & 0x55555555;
	return x;
}
//---------------------------------------------------------------------

constexpr size_t TREE_DIMENSIONS = 2;

template<typename T>
DEM_FORCE_INLINE T MortonLCA(T MortonCode1, T MortonCode2) noexcept
{
	// Shrink longer code so that both codes represent nodes on the same level
	const auto Bits1 = Math::BitWidth(MortonCode1);
	const auto Bits2 = Math::BitWidth(MortonCode2);
	if (Bits1 < Bits2)
		MortonCode2 >>= (Bits2 - Bits1);
	else
		MortonCode1 >>= (Bits1 - Bits2);

	// LCA is the equal prefix of both nodes. Find the place where equality breaks.
	auto HighestUnequalBit = Math::BitWidth(MortonCode1 ^ MortonCode2);

	// Each level uses TREE_DIMENSIONS bits and we must shift by whole levels
	if constexpr (TREE_DIMENSIONS == 2)
		HighestUnequalBit = Math::CeilToEven(HighestUnequalBit);
	else if constexpr (Math::IsPow2(TREE_DIMENSIONS))
		HighestUnequalBit = Math::CeilToMultipleOfPow2(HighestUnequalBit, TREE_DIMENSIONS);
	else
		HighestUnequalBit = Math::CeilToMultiple(HighestUnequalBit, TREE_DIMENSIONS);

	// Shift any of codes to obtain the common prefix which is the LCA of two nodes
	return MortonCode1 >> HighestUnequalBit;
}
//---------------------------------------------------------------------

/* //!!!clever Morton2_32 on x64! 16 bits are enough for any quadtree. So it is a pure win on x64!
uint32 Morton2(uint32 x, uint32 y)
{
// Merge the two 16-bit inputs into one 32-bit value
uint32 xy = (y << 16) + x;
// Separate bits of 32-bit value by one, giving 64-bit value
uint64 t = Part1By1_64BitOutput(xy);
// Interleave the top bits (y) with the bottom bits (x)
return (uint32)((t >> 31) + (t & 0x0ffffffff));
}
*/

//!!!DBG TMP! Move out of here!
// LinearTree<Dim, MaxDepth, IndexType/bits> - static class, could be a namespace but needs templating
// constexpr constants for fast calculations, e.g. float multiplier for the size of smallest node etc, or methods like CalcNodeSize(RootSize, Depth)
//bool IsParent()/IsChild()
//bool HasIntersection() - for loose need special algorithm, for non-loose it is IsParent/IsChild
//u8 GetDepth(Morton) - may need std::bit_ceil or bsr or lzcnt (faster than bsr) https://github.com/mackron/refcode/blob/master/lzcnt.c	
//uint GetX/Y/Z(Morton), also uint GetAxisValue(Morton, AxisIndex)
//Bounds GetBounds(Morton) - integer [L, R) for each axis in halves of the deepest node size. Clamp negative loose part of 0th node to 0? Or can use signed ints.
//Morton FindCommonParent() - shift longer one to be the same length as shorter one, then compare with & or ^? Matching part is a common parent.
// BFSToDFS, DFSToBFS index conversion
// Also can add a function ForEachIntersectingLooseNode(NodeA, [](){}), at each level will determine a range and iterate it
bool HasLooseIntersection(/*Bounds, Morton*/) noexcept
{
	// Depth = GetDepth(Morton);
	// InvDepth = MaxDepth - Depth;
	// HalfSize = 1 << InvDepth;    // LUT[Depth]?

	// for AxisIndex = 0..2
	//   Pos = GetAxisValue(Morton, AxisIndex);
	//   HalfPos = (Pos << 1) >> InvDepth; // LUT[Depth] for (InvDepth - 1)?
	//   Left = HalfPos - HalfSize;
	//   if Bounds.Max[AxisIndex] <= Left return false; // Could do Bounds.MaxX + HalfSize <= HalfPos to preserve unsigned math usage
	//   Rigth = Left + (HalfSize << 2)
	//   if Bounds.Min[AxisIndex] >= Rigth return false;

	return true;

	//???can try to solve inequations to know range of intersecting parents at each level based on depth difference and node pos?
	//for solving, all bit shifts can be turned into normal math ops, then turn back into code and optimize
}
//---------------------------------------------------------------------

void CSPS::Init(const vector3& Center, const vector3& Size, U8 HierarchyDepth)
{
	SceneMinY = Center.y - Size.y * 0.5f;
	SceneMaxY = Center.y + Size.y * 0.5f;
	QuadTree.Build(Center.x, Center.z, Size.x, Size.z, HierarchyDepth);

	///////// NEW RENDER /////////
	_MaxDepth = HierarchyDepth;
	_WorldBounds.Set(Center, Size);

	// Create a root node. This simplifies object insertion logic.
	// Set object count to fake 1 to keep the root alive forever.
	auto& Root = *_TreeNodes.emplace();
	Root.MortonCode = 1;
	Root.ParentIndex = NO_NODE;
	Root.SubtreeObjectCount = 1;
	_MortonToIndex.emplace(1, 0);
}
//---------------------------------------------------------------------

CSPSRecord* CSPS::AddRecord(const CAABB& GlobalBox, CNodeAttribute* pUserData)
{
	CSPSRecord* pRecord = RecordPool.Construct();
	pRecord->GlobalBox = GlobalBox;
	pRecord->pUserData = pUserData;
	pRecord->pPrev = nullptr;
	pRecord->pNext = nullptr;
	float CenterX, CenterZ, HalfSizeX, HalfSizeZ;
	GetDimensions(GlobalBox, CenterX, CenterZ, HalfSizeX, HalfSizeZ);
	QuadTree.AddObject(pRecord, CenterX, CenterZ, HalfSizeX, HalfSizeZ, pRecord->pSPSNode);

	///////// NEW RENDER /////////
	//!!!TODO: skip insertion if oversized or if outside root node, insert to deepest level if AABB is zero sized at any dimension!
	//???for moving objects, can benefit from knowing the previous node on reinsertion?

	//???store world bounds as center & half-extents? could help saving some calculations.
	const float RootSizeX = _WorldBounds.Max.x - _WorldBounds.Min.x;
	const float RootSizeZ = _WorldBounds.Max.z - _WorldBounds.Min.z;

	// Our level is where the non-loose node size is not less than our size in any of dimensions.
	// Since dividing full size on half size, we additionally divide result by 2.
	const auto HighestShare = std::min(static_cast<U32>(RootSizeX / HalfSizeX), static_cast<U32>(RootSizeZ / HalfSizeZ)) >> 1;
	const auto NodeSizeCoeff = std::min(Math::NextPow2(HighestShare), static_cast<U32>(1 << (_MaxDepth - 1)));

	// FIXME: can calc something like int(center-worldcenter) and shift with Depth?
	const auto Col = static_cast<U32>((CenterX - _WorldBounds.Min.x) * NodeSizeCoeff / RootSizeX);
	const auto Row = static_cast<U32>((CenterZ - _WorldBounds.Min.z) * NodeSizeCoeff / RootSizeZ);

	// NodeSizeCoeff is offset by Depth. Its square is offset 2x bits, making a room for 2D Morton code.
	const U32 NodeMortonCode = (NodeSizeCoeff * NodeSizeCoeff) | MortonCode2(Col) | (MortonCode2(Row) << 1);

	// Find the deepest existing parent. The root always exists as a fallback.
	U32 MissingNodes = 0;
	auto CurrMortonCode = NodeMortonCode;
	auto It = _MortonToIndex.find(CurrMortonCode);
	while (It == _MortonToIndex.end())
	{
		++MissingNodes;
		CurrMortonCode >>= TREE_DIMENSIONS;
		It = _MortonToIndex.find(CurrMortonCode);
	}

	const auto ExistingNodeIndex = It->second;

	// Increment all existing nodes' object counts
	auto NodeIndex = ExistingNodeIndex;
	while (NodeIndex != NO_NODE)
	{
		auto& Node = _TreeNodes[NodeIndex];
		++Node.SubtreeObjectCount;
		NodeIndex = Node.ParentIndex;
	}

	if (MissingNodes)
	{
		// Create missing nodes
		auto ParentIndex = ExistingNodeIndex;
		auto FreeIndex = _TreeNodes.first_free_index(ParentIndex);
		while (true)
		{
			--MissingNodes;
			const auto MortonCode = NodeMortonCode >> (MissingNodes * TREE_DIMENSIONS);
			const auto NextFreeIndex = _TreeNodes.next_free_index(FreeIndex);

			auto It = _TreeNodes.emplace_at_free(FreeIndex);
			It->MortonCode = MortonCode;
			It->ParentIndex = ParentIndex;
			It->SubtreeObjectCount = 1;
			_MortonToIndex.emplace(MortonCode, It.get_index());

			if (!MissingNodes)
			{
				pRecord->NodeIndex = It.get_index();
				break;
			}

			ParentIndex = It.get_index();
			FreeIndex = NextFreeIndex;
		}
	}
	else
	{
		pRecord->NodeIndex = ExistingNodeIndex;
	}

	pRecord->NodeMortonCode = NodeMortonCode;

	//////////////////////////////

	return pRecord;
}
//---------------------------------------------------------------------

void CSPS::UpdateRecord(CSPSRecord* pRecord)
{
	float CenterX, CenterZ, HalfSizeX, HalfSizeZ;
	GetDimensions(pRecord->GlobalBox, CenterX, CenterZ, HalfSizeX, HalfSizeZ);
	QuadTree.UpdateHandle(CSPSCell::CIterator(pRecord), CenterX, CenterZ, HalfSizeX, HalfSizeZ, pRecord->pSPSNode);

	///////// NEW RENDER /////////
	//!!!TODO: call UpdateRecord only when AABB changes! Check in the calling code!
	//!!!TODO: skip insertion if oversized or if outside root node, insert to deepest level if AABB is zero sized at any dimension!
	//???for moving objects, can benefit from knowing the previous node on reinsertion?
	constexpr size_t TREE_DIMENSIONS = 2;

	//???store world bounds as center & half-extents? could help saving some calculations.
	const float RootSizeX = _WorldBounds.Max.x - _WorldBounds.Min.x;
	const float RootSizeZ = _WorldBounds.Max.z - _WorldBounds.Min.z;

	// Our level is where the non-loose node size is not less than our size in any of dimensions.
	// Since dividing full size on half size, we additionally divide result by 2.
	const auto HighestShare = std::min(static_cast<U32>(RootSizeX / HalfSizeX), static_cast<U32>(RootSizeZ / HalfSizeZ)) >> 1;
	const auto NodeSizeCoeff = std::min(Math::NextPow2(HighestShare), static_cast<U32>(1 << (_MaxDepth - 1)));

	// FIXME: can calc something like int(center-worldcenter) and shift with Depth?
	const auto Col = static_cast<U32>((CenterX - _WorldBounds.Min.x) * NodeSizeCoeff / RootSizeX);
	const auto Row = static_cast<U32>((CenterZ - _WorldBounds.Min.z) * NodeSizeCoeff / RootSizeZ);

	// NodeSizeCoeff is offset by Depth. Its square is offset 2x bits, making a room for 2D Morton code.
	const U32 NodeMortonCode = (NodeSizeCoeff * NodeSizeCoeff) | MortonCode2(Col) | (MortonCode2(Row) << 1);

	//!!!DBG TMP! UNCOMMENT!
	//if (pRecord->NodeMortonCode == NodeMortonCode) return;

	// Find LCA of the current and the new node
	const auto LCAMortonCode = MortonLCA(pRecord->NodeMortonCode, NodeMortonCode);


	//shrink longer to shorter
	//XOR
	//find first nonzero bit
	//shrink any of codes to shift out that bit
	//LCA found!

	// from current node to LCA exclusive
	//   decrement object count
	//   if object count became 0, erase in _MortonToIndex and _TreeNodes
	// from new node to LCA exclusive
	//   create missing nodes, then
	//   increment count of existing nodes
	// update node indices in the object record
}
//---------------------------------------------------------------------

void CSPS::RemoveRecord(CSPSRecord* pRecord)
{
	if (pRecord->pSPSNode) pRecord->pSPSNode->RemoveByHandle(CSPSCell::CIterator(pRecord));
	RecordPool.Destroy(pRecord);

	///////// NEW RENDER /////////
	auto NodeIndex = pRecord->NodeIndex;
	while (NodeIndex != NO_NODE)
	{
		auto& Node = _TreeNodes[NodeIndex];
		auto ParentIndex = Node.ParentIndex;
		if (--Node.SubtreeObjectCount == 0)
		{
			_MortonToIndex.erase(Node.MortonCode);
			_TreeNodes.erase(NodeIndex);
		}
		NodeIndex = ParentIndex;
	}

	pRecord->NodeMortonCode = 0;
	pRecord->NodeIndex = NO_NODE;
}
//---------------------------------------------------------------------

void CSPS::QueryObjectsInsideFrustum(const matrix44& ViewProj, CArray<CNodeAttribute*>& OutObjects) const
{
	// TODO: oversized and always visible are two different things. Oversized must be AABB-tested, always visible must not.
	if (OversizedObjects.GetCount())
		OutObjects.AddArray(OversizedObjects);

	CSPSNode* pRootNode = QuadTree.GetRootNode();
	if (pRootNode && pRootNode->GetTotalObjCount())
		QueryObjectsInsideFrustum(pRootNode, ViewProj, OutObjects, EClipStatus::Clipped);
}
//---------------------------------------------------------------------

void CSPS::QueryObjectsInsideFrustum(CSPSNode* pNode, const matrix44& ViewProj, CArray<CNodeAttribute*>& OutObjects, EClipStatus Clip) const
{
	n_assert_dbg(pNode && pNode->GetTotalObjCount() && Clip != EClipStatus::Outside);

	if (Clip == EClipStatus::Clipped)
	{
		CAABB NodeBox;
		pNode->GetBounds(NodeBox); //!!!can pass node box as arg and calculate for children, this will save some calculations!
		NodeBox.Min.y = SceneMinY;
		NodeBox.Max.y = SceneMaxY;
		Clip = NodeBox.GetClipStatus(ViewProj);
		if (Clip == EClipStatus::Outside) return;
		if (Clip == EClipStatus::Inside)
		{
			// Make room for all underlying objects, including those in child nodes
			UPTR MinRoomRequired = OutObjects.GetCount() + pNode->GetTotalObjCount();
			if (OutObjects.GetAllocSize() < MinRoomRequired)
				OutObjects.Resize(MinRoomRequired);
		}
	}

	if (!pNode->Data.IsEmpty())
	{
		CSPSCell::CIterator ItObj = pNode->Data.Begin();
		CSPSCell::CIterator ItEnd = pNode->Data.End();
		if (Clip == EClipStatus::Inside)
		{
			for (; ItObj != ItEnd; ++ItObj)
				OutObjects.Add((*ItObj)->pUserData);
		}
		else // Clipped
		{
			for (; ItObj != ItEnd; ++ItObj)
				if ((*ItObj)->GlobalBox.GetClipStatus(ViewProj) != EClipStatus::Outside)
					OutObjects.Add((*ItObj)->pUserData);
		}
	}

	if (pNode->HasChildren())
		for (UPTR i = 0; i < 4; ++i)
		{
			CSPSNode* pChildNode = pNode->GetChild(i);
			if (pChildNode->GetTotalObjCount())
				QueryObjectsInsideFrustum(pChildNode, ViewProj, OutObjects, Clip);
		}
}
//---------------------------------------------------------------------

}
