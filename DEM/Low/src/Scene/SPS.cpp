#include <Scene/SPS.h>
#include <Math/Math.h>
#include <acl/math/vector4_32.h>
#include <acl/math/affine_matrix_32.h>

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

DEM_FORCE_INLINE U32 MortonCode2(U16 x, U16 y) noexcept
{
#if DEM_X64
	const U32 Mixed = (static_cast<U32>(y) << 16) | x;
	const U64 MixedParted = Math::PartBits1By1(Mixed);
	return static_cast<U32>((MixedParted >> 31) | (MixedParted & 0x0ffffffff));
#else
	return Math::PartBits1By1(x) | (Math::PartBits1By1(y) << 1);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U64 MortonCode2(U32 x, U32 y) noexcept
{
	return Math::PartBits1By1(x) | (Math::PartBits1By1(y) << 1);
}
//---------------------------------------------------------------------

constexpr size_t TREE_DIMENSIONS = 2;

// Finds the Least Common Ancestor of two nodes represented by Morton codes
template<size_t DIMENSIONS, typename T>
DEM_FORCE_INLINE T MortonLCA(T MortonCodeA, T MortonCodeB) noexcept
{
	// Shrink longer code so that both codes represent nodes on the same level
	const auto Bits1 = Math::BitWidth(MortonCodeA);
	const auto Bits2 = Math::BitWidth(MortonCodeB);
	if (Bits1 < Bits2)
		MortonCodeB >>= (Bits2 - Bits1);
	else
		MortonCodeA >>= (Bits1 - Bits2);

	// LCA is the equal prefix of both nodes. Find the place where equality breaks.
	auto HighestUnequalBit = Math::BitWidth(MortonCodeA ^ MortonCodeB);

	// Each level uses DIMENSIONS bits and we must shift by whole levels
	if constexpr (DIMENSIONS == 2)
		HighestUnequalBit = Math::CeilToEven(HighestUnequalBit);
	else if constexpr (Math::IsPow2(DIMENSIONS))
		HighestUnequalBit = Math::CeilToMultipleOfPow2(HighestUnequalBit, DIMENSIONS);
	else
		HighestUnequalBit = Math::CeilToMultiple(HighestUnequalBit, DIMENSIONS);

	// Shift any of codes to obtain the common prefix which is the LCA of two nodes
	return MortonCodeA >> HighestUnequalBit;
}
//---------------------------------------------------------------------

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
// Also can add a function ForEachIntersectingLooseNode(NodeA, [](){}), at each level will determine a range and iterate it.
//
// for overlap test in a loose tree need to check neighbours, and therefore need to get the deepest common ancestor of 2 nodes with Morton codes
// in a loose tree need to check our node and all neighbours, including diagonal!
// for lights-objects overlap it is easier to scan through all lights and check HasLooseIntersection of light tree node against the object tree node
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

void CSPS::Init(const vector3& Center, float Size, U8 HierarchyDepth)
{
	SceneMinY = Center.y - Size * 0.5f;
	SceneMaxY = Center.y + Size * 0.5f;
	QuadTree.Build(Center.x, Center.z, Size, Size, HierarchyDepth);

	///////// NEW RENDER /////////
	_MaxDepth = 12; // Required Morton bits = 2 * _MaxDepth + 1 or 3 * _MaxDepth + 1, col/row index bits = _MaxDepth
	_WorldCenter = Center;
	_WorldHalfExtent = Size * 0.5f;

	// Create a root node. This simplifies object insertion logic.
	// Set object count to fake 1 to keep the root alive forever.
	auto& Root = *_TreeNodes.emplace();
	Root.MortonCode = 1;
	Root.ParentIndex = NO_NODE;
	Root.SubtreeObjectCount = 1;
	_MortonToIndex.emplace(1, 0);
}
//---------------------------------------------------------------------

U32 CSPS::CalculateQuadtreeMortonCode(float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ) const noexcept
{
	// Our level is where the non-loose node size is not less than our size in any of dimensions.
	// Too small and degenerate AABBs sink to the deepest possible level to save us from division errors.
	U32 NodeSizeCoeff = static_cast<U32>(1 << (_MaxDepth - 1));
	if (HalfSizeX > 0.0001f && HalfSizeZ > 0.0001f)
	{
		const auto HighestSharePow2 = Math::NextPow2(static_cast<U32>(_WorldHalfExtent / std::max(HalfSizeX, HalfSizeZ)));
		if (NodeSizeCoeff > HighestSharePow2) NodeSizeCoeff = HighestSharePow2;
	}

	const float CellCoeff = static_cast<float>(NodeSizeCoeff) / (_WorldHalfExtent + _WorldHalfExtent);

	// TODO: use SIMD. Can almost unify quadtree and octree with that.
	const auto Col = static_cast<U16>((CenterX - _WorldCenter.x + _WorldHalfExtent) * CellCoeff);
	const auto Row = static_cast<U16>((CenterZ - _WorldCenter.z + _WorldHalfExtent) * CellCoeff);

	// NodeSizeCoeff bit is offset by Depth bits. Its square is offset 2x bits, making a room for 2D Morton code.
	return (NodeSizeCoeff * NodeSizeCoeff) | MortonCode2(Col, Row);
}
//---------------------------------------------------------------------

U32 CSPS::CreateNode(U32 FreeIndex, U32 MortonCode, U32 ParentIndex)
{
	auto ItNew = _TreeNodes.emplace_at_free(FreeIndex);
	ItNew->MortonCode = MortonCode;
	ItNew->ParentIndex = ParentIndex;
	ItNew->SubtreeObjectCount = 1;

	if (_MappingPool.empty())
	{
		_MortonToIndex.emplace(MortonCode, ItNew.get_index());
	}
	else
	{
		auto& MappingNode = _MappingPool.back();
		MappingNode.key() = MortonCode;
		MappingNode.mapped() = ItNew.get_index();
		_MortonToIndex.insert(std::move(MappingNode));
		_MappingPool.pop_back();
	}

	return ItNew.get_index();
}
//---------------------------------------------------------------------

U32 CSPS::AddSingleObject(U32 NodeMortonCode, U32 StopMortonCode)
{
	// Find the deepest existing parent. The root always exists as a fallback.
	U32 MissingNodes = 0;
	auto CurrMortonCode = NodeMortonCode;
	auto ItExisting = _MortonToIndex.find(CurrMortonCode);
	while (ItExisting == _MortonToIndex.end())
	{
		++MissingNodes;
		CurrMortonCode >>= TREE_DIMENSIONS;
		ItExisting = _MortonToIndex.find(CurrMortonCode);
	}

	const auto ExistingNodeIndex = ItExisting->second;

	// Increment existing nodes' object counts
	auto NodeIndex = ExistingNodeIndex;
	while (CurrMortonCode != StopMortonCode)
	{
		auto& Node = _TreeNodes[NodeIndex];
		++Node.SubtreeObjectCount;
		NodeIndex = Node.ParentIndex;
		CurrMortonCode >>= TREE_DIMENSIONS;
	}

	if (!MissingNodes) return ExistingNodeIndex;

	// Create missing parent nodes
	auto ParentIndex = ExistingNodeIndex;
	auto FreeIndex = _TreeNodes.first_free_index(ParentIndex);
	while (--MissingNodes)
	{
		const auto NextFreeIndex = _TreeNodes.next_free_index(FreeIndex);
		ParentIndex = CreateNode(FreeIndex, NodeMortonCode >> (MissingNodes * TREE_DIMENSIONS), ParentIndex);
		FreeIndex = NextFreeIndex;
	}

	// Create missing leaf node
	return CreateNode(FreeIndex, NodeMortonCode, ParentIndex);
}
//---------------------------------------------------------------------

void CSPS::RemoveSingleObject(U32 NodeIndex, U32 NodeMortonCode, U32 StopMortonCode)
{
	while (NodeMortonCode != StopMortonCode)
	{
		auto& Node = _TreeNodes[NodeIndex];
		auto ParentIndex = Node.ParentIndex;
		if (--Node.SubtreeObjectCount == 0)
		{
			_MappingPool.push_back(_MortonToIndex.extract(Node.MortonCode));
			_TreeNodes.erase(NodeIndex);
		}
		NodeIndex = ParentIndex;
		NodeMortonCode >>= TREE_DIMENSIONS;
	}
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
	//!!!TODO: skip insertion if oversized or if outside root node ???store oversized objects in the root or at 0?
	const U32 NodeMortonCode = CalculateQuadtreeMortonCode(CenterX, CenterZ, HalfSizeX, HalfSizeZ);
	pRecord->NodeIndex = AddSingleObject(NodeMortonCode, 0);
	pRecord->NodeMortonCode = NodeMortonCode;
	//???TODO: remember in pRecord the number of the last frame where the node changed?
	//////////////////////////////

	return pRecord;
}
//---------------------------------------------------------------------

//!!!TODO: call UpdateRecord only when AABB changes! Check in the calling code!
void CSPS::UpdateRecord(CSPSRecord* pRecord)
{
	float CenterX, CenterZ, HalfSizeX, HalfSizeZ;
	GetDimensions(pRecord->GlobalBox, CenterX, CenterZ, HalfSizeX, HalfSizeZ);
	QuadTree.UpdateHandle(CSPSCell::CIterator(pRecord), CenterX, CenterZ, HalfSizeX, HalfSizeZ, pRecord->pSPSNode);

	///////// NEW RENDER /////////
	const U32 NodeMortonCode = CalculateQuadtreeMortonCode(CenterX, CenterZ, HalfSizeX, HalfSizeZ);

	if (pRecord->NodeMortonCode == NodeMortonCode) return;

	const auto LCAMortonCode = MortonLCA<TREE_DIMENSIONS>(pRecord->NodeMortonCode, NodeMortonCode);

	RemoveSingleObject(pRecord->NodeIndex, pRecord->NodeMortonCode, LCAMortonCode);
	pRecord->NodeIndex = AddSingleObject(NodeMortonCode, LCAMortonCode);
	pRecord->NodeMortonCode = NodeMortonCode;
	//???TODO: remember in pRecord the number of the last frame where the node changed?
	//////////////////////////////
}
//---------------------------------------------------------------------

void CSPS::RemoveRecord(CSPSRecord* pRecord)
{
	if (pRecord->pSPSNode) pRecord->pSPSNode->RemoveByHandle(CSPSCell::CIterator(pRecord));
	RecordPool.Destroy(pRecord);

	///////// NEW RENDER /////////
	RemoveSingleObject(pRecord->NodeIndex, pRecord->NodeMortonCode, 0);
	pRecord->NodeMortonCode = 0;
	pRecord->NodeIndex = NO_NODE;
	//////////////////////////////
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

//!!!TODO: to Math!
struct CSIMDFourPlanes
{
	acl::Vector4_32	X;
	acl::Vector4_32	Y;
	acl::Vector4_32	Z;
	acl::Vector4_32	W;
};
// FIXME: need 2 iterations? matches acl::sqrt_reciprocal on 1. Maybe I was lucky with data.
DEM_FORCE_INLINE acl::Vector4_32 vector_sqrt_reciprocal(acl::Vector4_32 input)
{
#if defined(ACL_SSE2_INTRINSICS)
	// Perform two passes of Newton-Raphson iteration on the hardware estimate
	__m128 half = _mm_set_ps1(0.5F);
	__m128 input_half_v = _mm_mul_ps(input, half);
	__m128 x0 = _mm_rsqrt_ps(input);

	// First iteration
	__m128 x1 = _mm_mul_ps(x0, x0);
	x1 = _mm_sub_ps(half, _mm_mul_ps(input_half_v, x1));
	x1 = _mm_add_ps(_mm_mul_ps(x0, x1), x0);

	// Second iteration
	__m128 x2 = _mm_mul_ps(x1, x1);
	x2 = _mm_sub_ps(half, _mm_mul_ps(input_half_v, x2));
	x2 = _mm_add_ps(_mm_mul_ps(x1, x2), x1);

	return x2;
#else
	return acl::vector_set(
		1.0F / sqrt(acl::vector_get_x(input)),
		1.0F / sqrt(acl::vector_get_y(input)),
		1.0F / sqrt(acl::vector_get_z(input)),
		1.0F / sqrt(acl::vector_get_w(input)));
#endif
}
//---------------------------------------------------------------------

// See e.g. https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
DEM_FORCE_INLINE CSIMDFourPlanes PlanesFromMatrixLRBT(const matrix44& m)
{
	auto NegAs = acl::vector_add(acl::vector_set(-m.m[0][0], m.m[0][0], -m.m[0][1], m.m[0][1]), acl::vector_set(-m.m[0][3]));
	auto NegBs = acl::vector_add(acl::vector_set(-m.m[1][0], m.m[1][0], -m.m[1][1], m.m[1][1]), acl::vector_set(-m.m[1][3]));
	auto NegCs = acl::vector_add(acl::vector_set(-m.m[2][0], m.m[2][0], -m.m[2][1], m.m[2][1]), acl::vector_set(-m.m[2][3]));
	auto Ds = acl::vector_add(acl::vector_set(m.m[3][0], -m.m[3][0], m.m[3][1], -m.m[3][1]), acl::vector_set(m.m[3][3]));

	auto InvLengths = acl::vector_mul(NegAs, NegAs);
	InvLengths = acl::vector_mul_add(NegBs, NegBs, InvLengths);
	InvLengths = acl::vector_mul_add(NegCs, NegCs, InvLengths);
	InvLengths = vector_sqrt_reciprocal(InvLengths);

	auto PlanesX = acl::vector_mul(NegAs, InvLengths);
	auto PlanesY = acl::vector_mul(NegBs, InvLengths);
	auto PlanesZ = acl::vector_mul(NegCs, InvLengths);
	auto PlanesW = acl::vector_mul(Ds, InvLengths);
	return CSIMDFourPlanes{ PlanesX, PlanesY, PlanesZ, PlanesW };
}
//---------------------------------------------------------------------

// https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
DEM_FORCE_INLINE CSIMDFourPlanes PlanesFromMatrixNF(const matrix44& m)
{
	// We use D3D style projection matrix, near Z limit is 0 instead of OpenGL's -W
	const float ANear = m.m[0][2];
	const float BNear = m.m[1][2];
	const float CNear = m.m[2][2];
	const float DNear = m.m[3][2];
	const float InvLengthNear = acl::sqrt_reciprocal(ANear * ANear + BNear * BNear + CNear * CNear);

	const float AFar = m.m[0][3] - m.m[0][2];
	const float BFar = m.m[1][3] - m.m[1][2];
	const float CFar = m.m[2][3] - m.m[2][2];
	const float DFar = m.m[3][3] - m.m[3][2];
	const float InvLengthFar = acl::sqrt_reciprocal(AFar * AFar + BFar * BFar + CFar * CFar);

	const auto InvLengths = acl::vector_set(InvLengthNear, InvLengthFar, InvLengthNear, InvLengthNear);

	auto PlanesX = acl::vector_mul(acl::vector_set(-ANear, -AFar, -ANear, -ANear), InvLengths);
	auto PlanesY = acl::vector_mul(acl::vector_set(-BNear, -BFar, -BNear, -BNear), InvLengths);
	auto PlanesZ = acl::vector_mul(acl::vector_set(-CNear, -CFar, -CNear, -CNear), InvLengths);
	auto PlanesW = acl::vector_mul(acl::vector_set(DNear, DFar, DNear, DNear), InvLengths);
	return CSIMDFourPlanes{ PlanesX, PlanesY, PlanesZ, PlanesW };
}
//---------------------------------------------------------------------

void CSPS::TestSpatialTreeVisibility(const matrix44& ViewProj, std::vector<bool>& NodeVisibility) const
{
	n_assert2_dbg(!_TreeNodes.empty(), "CSPS::TestSpatialTreeVisibility() should not be called before CSPS::Init()!");

	// Cell can have 4 frustum culling states, for which 2 bits are enough. And we use clever meanings for
	// individual bits inspired by UE, 'is inside' and 'is outside', so that we have the next states:
	// not checked (00), completely inside (01), completely outside (10), partially inside (11).
	NodeVisibility.resize(_TreeNodes.sparse_size() * 2, false);

	/*
	float NodeSizeX, NodeSizeZ;

	const vector2& RootSize = pOwner->GetSize();

	if (Level > 0)
	{
		float SizeCoeff = 1.f / (1 << Level);
		NodeSizeX = RootSize.x * SizeCoeff;
		NodeSizeZ = RootSize.y * SizeCoeff;
	}
	else
	{
		NodeSizeX = RootSize.x;
		NodeSizeZ = RootSize.y;
	}

	Box.Min.x = pOwner->Center.x + Col * NodeSizeX - RootSize.x * 0.5f;
	Box.Min.z = pOwner->Center.y + Row * NodeSizeZ - RootSize.y * 0.5f;
	Box.Max.x = Box.Min.x + NodeSizeX;
	Box.Max.z = Box.Min.z + NodeSizeZ;
	Box.Min.y = SceneMinY;
	Box.Max.y = SceneMaxY;
	*/

	const CSIMDFourPlanes PlanesLRBT = PlanesFromMatrixLRBT(ViewProj);
	//???need? check directly w (clip-space z) against znear & zfar? const CSIMDFourPlanes PlanesNF = PlanesFromMatrixNF(ViewProj);

	//!!!DBG TMP!
	//if (acl::vector_all_greater_equal(PlanesLRBT.X, PlanesLRBT.Y)) return;
	//if (acl::vector_all_greater_equal(PlanesLRBT.Z, PlanesLRBT.W)) return;

	// build frustum planes for SIMD from ViewProj
	// to simplify the test, use the fact that we have the same extent in all directions for a node
	// test inside and outside at once

	// Process the root outside the loop to simplify conditions inside
	auto ItNode = _TreeNodes.cbegin();
	//const bool IsVisible = IntersectBox8Plane(NodeBounds.Center, NodeBounds.Extent, View.ViewFrustum.PermutedPlanes.GetData());
	//NodeVisibility[0] = IsVisible
	//NodeVisibility[1] = IsVisible && TestBoxFrustum(NodeBounds.Center, NodeBounds.Extent).GetOutside();
	++ItNode;

	//???!!!TODO: don't recalculate cached values from previous frames?! recalc only nodes with 'not checked' state?
	//???use 00 for invisibles and write only visibility because invisibles are already memset?
	for (; ItNode != _TreeNodes.cend(); ++ItNode)
	{
		//!!!DBG TMP! CSparseArray2 guarantees the order, but we check twice.
		n_assert_dbg(ItNode->ParentIndex < ItNode.get_index());

		const bool IsParentInside = NodeVisibility[ItNode->ParentIndex * 2];
		const bool IsParentOutside = NodeVisibility[ItNode->ParentIndex * 2 + 1];
		if (IsParentInside != IsParentOutside)
		{
			// If the parent is completely visible or completely invisible, all its children have the same state
			// TODO: use special array of 2-bit values to optimize number of read and write operations?
			NodeVisibility[ItNode.get_index() * 2] = IsParentInside;
			NodeVisibility[ItNode.get_index() * 2 + 1] = IsParentOutside;
		}
		else
		{
			// If the parent is partially visible
			//const bool IsVisible = IntersectBox8Plane(NodeBounds.Center, NodeBounds.Extent, View.ViewFrustum.PermutedPlanes.GetData());
			//NodeVisibility[ItNode.get_index() * 2] = IsVisible
			//NodeVisibility[ItNode.get_index() * 2 + 1] = IsVisible && TestBoxFrustum(NodeBounds.Center, NodeBounds.Extent).GetOutside();
		}
	}
}
//---------------------------------------------------------------------

}
