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

template<size_t DIMENSIONS, typename T>
DEM_FORCE_INLINE T GetDepthLevel(T MortonCode) noexcept
{
	return Math::BitWidth(MortonCode) / DIMENSIONS;
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
	n_assert2_dbg(Size >= 0.f, "CSPS::Init() > negative world extent is not allowed!");

	SceneMinY = Center.y - Size * 0.5f;
	SceneMaxY = Center.y + Size * 0.5f;
	QuadTree.Build(Center.x, Center.z, Size, Size, HierarchyDepth);

	///////// NEW RENDER /////////
	_MaxDepth = std::min(static_cast<U8>(12), TREE_MAX_DEPTH); // TODO: pass as an arg, assert and clamp if requested more than possible.
	_WorldCenter = Center; //_WorldBounds = acl::vector_set(Center.x, Center.y, Center.z, 1.f);
	_WorldExtent = Size * 0.5f;
	_InvWorldSize = 1.f / Size;

	// Create a root node. This simplifies object insertion logic.
	// Set object count to fake 1 to keep the root alive forever.
	auto& Root = *_TreeNodes.emplace();
	Root.MortonCode = 1;
	Root.ParentIndex = NO_NODE;
	Root.SubtreeObjectCount = 1;
	_MortonToIndex.emplace(1, 0);
}
//---------------------------------------------------------------------

TMorton CSPS::CalculateMortonCode(const CAABB& AABB) const noexcept
{
	// FIXME: store world bounds in a SIMD vector!
	const auto WorldCenter = acl::vector_set(_WorldCenter.x, _WorldCenter.y, _WorldCenter.z);
	const auto WorldExtent = acl::vector_set(_WorldExtent);

	// FIXME: can initially store AABB as SIMD center & extent?
	const auto HalfMin = acl::vector_mul(acl::vector_set(AABB.Min.x, AABB.Min.y, AABB.Min.z), 0.5f);
	const auto HalfMax = acl::vector_mul(acl::vector_set(AABB.Max.x, AABB.Max.y, AABB.Max.z), 0.5f);
	const auto Center = acl::vector_add(HalfMax, HalfMin);
	const auto Extent = acl::vector_sub(HalfMax, HalfMin);

	// Our level is where the non-loose node size is not less than our size in any of dimensions.
	// Too small and degenerate AABBs sink to the deepest possible level to save us from division errors.
	TMorton NodeSizeCoeff = static_cast<TMorton>(1 << (_MaxDepth - 1));
	if (acl::vector_all_greater_equal3(Extent, acl::vector_set(0.0001f)))
	{
		// TODO: can make better with SIMD?
		const float MaxDim = std::max({ acl::vector_get_x(Extent), acl::vector_get_y(Extent), acl::vector_get_z(Extent) });
		const TMorton HighestSharePow2 = Math::NextPow2(static_cast<TMorton>(_WorldExtent / MaxDim));
		if (NodeSizeCoeff > HighestSharePow2) NodeSizeCoeff = HighestSharePow2;
	}

	const float CellCoeff = static_cast<float>(NodeSizeCoeff) * _InvWorldSize;
	const auto Cell = acl::vector_mul(acl::vector_add(acl::vector_sub(Center, WorldCenter), WorldExtent), CellCoeff); // (C - Wc + We) * Coeff

	const auto x = static_cast<TCellDim>(acl::vector_get_x(Cell));
	const auto y = static_cast<TCellDim>(acl::vector_get_y(Cell));
	const auto z = static_cast<TCellDim>(acl::vector_get_z(Cell));

	// NodeSizeCoeff bit is offset by Depth bits. Its pow(N) is a bit offset to N*Depth, making a room for N-dimensional Morton code.
	if constexpr (TREE_DIMENSIONS == 2)
		return (NodeSizeCoeff * NodeSizeCoeff) | Math::MortonCode2(x, z);
	else if constexpr (sizeof(TMorton) >= 8)
		return (NodeSizeCoeff * NodeSizeCoeff * NodeSizeCoeff) | Math::MortonCode3_21bit(x, y, z);
	else
		return (NodeSizeCoeff * NodeSizeCoeff * NodeSizeCoeff) | Math::MortonCode3_10bit(x, y, z);
}
//---------------------------------------------------------------------

U32 CSPS::CreateNode(U32 FreeIndex, TMorton MortonCode, U32 ParentIndex)
{
	// FIXME: store world bounds in a SIMD vector!
	const auto WorldCenter = acl::vector_set(_WorldCenter.x, _WorldCenter.y, _WorldCenter.z);

	// Unpack Morton code back into cell coords and depth
	const auto Bits = Math::BitWidth(MortonCode);
	const auto MortonCodeNoSentinel = MortonCode ^ (1 << (Bits - 1));
	TCellDim x = 0, y = 0, z = 0;
	if constexpr (TREE_DIMENSIONS == 2)
		Math::MortonDecode2(MortonCodeNoSentinel, x, z);
	else
		Math::MortonDecode3(MortonCodeNoSentinel, x, y, z);

	// Calculate node bounds - center and extent
	const float ExtentCoeff = 1.f / static_cast<float>(1 << (Bits / TREE_DIMENSIONS)); // 1 / 2^Depth
	const auto Cell = acl::vector_set(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	auto Center = acl::vector_mul_add(Cell, 2.f, acl::vector_set(1.f)); // A = 2 * xyz + 1
	Center = acl::vector_mul_add(Center, ExtentCoeff, acl::vector_set(-1.f)); // B = A * Ecoeff - 1.f
	Center = acl::vector_mul_add(Center, _WorldExtent, WorldCenter); // Center = B * We + Wc

	// Set extent coeff to W
	Center = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::A>(Center, acl::vector_set(ExtentCoeff));

	// Create a node
	auto ItNew = _TreeNodes.emplace_at_free(FreeIndex);
	ItNew->MortonCode = MortonCode;
	ItNew->ParentIndex = ParentIndex;
	ItNew->SubtreeObjectCount = 1;
	ItNew->Bounds = Center;

	if (_MortonToIndexPool.empty())
	{
		_MortonToIndex.emplace(MortonCode, ItNew.get_index());
	}
	else
	{
		auto& MappingNode = _MortonToIndexPool.back();
		MappingNode.key() = MortonCode;
		MappingNode.mapped() = ItNew.get_index();
		_MortonToIndex.insert(std::move(MappingNode));
		_MortonToIndexPool.pop_back();
	}

	return ItNew.get_index();
}
//---------------------------------------------------------------------

U32 CSPS::AddSingleObject(TMorton NodeMortonCode, TMorton StopMortonCode)
{
	if (!NodeMortonCode) return NO_NODE;

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

void CSPS::RemoveSingleObject(U32 NodeIndex, TMorton NodeMortonCode, TMorton StopMortonCode)
{
	while (NodeMortonCode != StopMortonCode)
	{
		auto& Node = _TreeNodes[NodeIndex];
		auto ParentIndex = Node.ParentIndex;
		if (--Node.SubtreeObjectCount == 0)
		{
			_MortonToIndexPool.push_back(_MortonToIndex.extract(Node.MortonCode));
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
	const auto NodeMortonCode = CalculateMortonCode(GlobalBox);
	pRecord->NodeIndex = AddSingleObject(NodeMortonCode, 0);
	pRecord->NodeMortonCode = NodeMortonCode;
	pRecord->BoundsVersion = 1;
	_Objects.emplace_hint(_Objects.cend(), _NextUID++, pRecord); //!!!TODO: insert (hint, std::move(node))

	// If this assert is ever triggered, compacting of existing UIDs may be implemented to keep fast insertions to the map end.
	// Compacting must change UIDs in _Objects and broadcast changes to all views. Try to make it sorted and preserve iterators to avoid logN searches.
	n_assert_dbg(_NextUID < std::numeric_limits<decltype(_NextUID)>().max());
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
	//!!!pass box as an arg! can skip update if the box is the same. Easy to compare using SIMD Center&Extent.
	const auto NodeMortonCode = CalculateMortonCode(pRecord->GlobalBox);

	if (pRecord->NodeMortonCode == NodeMortonCode) return;

	const auto LCAMortonCode = MortonLCA<TREE_DIMENSIONS>(pRecord->NodeMortonCode, NodeMortonCode);

	RemoveSingleObject(pRecord->NodeIndex, pRecord->NodeMortonCode, LCAMortonCode);
	pRecord->NodeIndex = AddSingleObject(NodeMortonCode, LCAMortonCode);
	pRecord->NodeMortonCode = NodeMortonCode;
	if (++pRecord->BoundsVersion == 0) ++pRecord->BoundsVersion; // 0 is a special invalid version
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
	//extract from _Objects to pool! By UID stored in the attr itself? Then need also to clear UID in the attr here.
	//!!!could also store iterator instead of UID in attr, it is less safe but O(1) extract instead of O(logN)! Clear all iterators before _Objects.clear() then!
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

// Test AABB vs frustum planes intersection for positive halfspace treated as 'inside'
static DEM_FORCE_INLINE U8 ClipCube(acl::Vector4_32Arg0 Bounds, acl::Vector4_32Arg1 ProjectedNegWorldExtent,
	acl::Vector4_32Arg2 LRBT_Nx, acl::Vector4_32Arg3 LRBT_Ny, acl::Vector4_32Arg4 LRBT_Nz, acl::Vector4_32Arg5 LRBT_w,
	acl::Vector4_32ArgN LookAxis, float NegWorldExtentAlongLookAxis, float NearPlane, float FarPlane)
{
	// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = acl::vector_mul_add(acl::vector_mix_xxxx(Bounds), LRBT_Nx, LRBT_w);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_yyyy(Bounds), LRBT_Ny, CenterDistance);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_zzzz(Bounds), LRBT_Nz, CenterDistance);

	// Projection radius of the most outside vertex (-r)
	const auto ProjectedNegExtent = acl::vector_mul(ProjectedNegWorldExtent, acl::vector_mix_wwww(Bounds));

	// Check intersection with LRTB planes
	bool HasVisiblePart = acl::vector_all_greater_equal(CenterDistance, ProjectedNegExtent); //!!!need strictly greater (RTM?)!
	bool HasInvisiblePart = false;
	if (HasVisiblePart)
	{
		// If inside LRTB, check intersection with NF planes
		const float CenterAlongLookAxis = acl::vector_dot3(LookAxis, Bounds);
		const float NegExtentAlongLookAxis = NegWorldExtentAlongLookAxis * acl::vector_get_w(Bounds);
		const float ClosestPoint = CenterAlongLookAxis + NegExtentAlongLookAxis;
		const float FarthestPoint = CenterAlongLookAxis - NegExtentAlongLookAxis;
		HasVisiblePart = (FarthestPoint > NearPlane && ClosestPoint < FarPlane);
		HasInvisiblePart = !HasVisiblePart || (FarthestPoint > FarPlane || ClosestPoint < NearPlane);
	}

	HasInvisiblePart = HasInvisiblePart || acl::vector_any_less_than(CenterDistance, acl::vector_neg(ProjectedNegExtent));

	return static_cast<U8>(HasVisiblePart) | (static_cast<U8>(HasInvisiblePart) << 1);
}
//---------------------------------------------------------------------

// See Real-Time Collision Detection 5.2.3
// See https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/
void CSPS::TestSpatialTreeVisibility(const matrix44& ViewProj, std::vector<bool>& NodeVisibility) const
{
	n_assert2_dbg(!_TreeNodes.empty(), "CSPS::TestSpatialTreeVisibility() should not be called before CSPS::Init()!");

	//const size_t CachedCount = NodeVisibility.size() / 2;

	// Cell can have 4 frustum culling states, for which 2 bits are enough. And we use clever meanings for
	// individual bits inspired by UE, 'is inside' and 'is outside', so that we have the next states:
	// not checked (00), completely inside (01), completely outside (10), partially inside (11).
	NodeVisibility.resize(_TreeNodes.sparse_size() * 2, false);

	// Extract Left, Right, Bottom & Top frustum planes using Gribb-Hartmann method.
	// Left = W + X, Right = W - X, Bottom = W + Y, Top = W - Y. Normals look inside the frustum, i.e. positive halfspace means 'inside'.
	// LRBT_w is a -d, i.e. -dot(PlaneNormal, PlaneOrigin). +w instead of -d allows us using a single fma instead of separate mul & sub in a box test.
	// See https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
	// See https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
	const auto LRBT_Nx = acl::vector_add(acl::vector_set(ViewProj.m[0][3]), acl::vector_set(ViewProj.m[0][0], -ViewProj.m[0][0], ViewProj.m[0][1], -ViewProj.m[0][1]));
	const auto LRBT_Ny = acl::vector_add(acl::vector_set(ViewProj.m[1][3]), acl::vector_set(ViewProj.m[1][0], -ViewProj.m[1][0], ViewProj.m[1][1], -ViewProj.m[1][1]));
	const auto LRBT_Nz = acl::vector_add(acl::vector_set(ViewProj.m[2][3]), acl::vector_set(ViewProj.m[2][0], -ViewProj.m[2][0], ViewProj.m[2][1], -ViewProj.m[2][1]));
	const auto LRBT_w =  acl::vector_add(acl::vector_set(ViewProj.m[3][3]), acl::vector_set(ViewProj.m[3][0], -ViewProj.m[3][0], ViewProj.m[3][1], -ViewProj.m[3][1]));

	// Projection radius of the most outside vertex (-r): -Ex * abs(Pnx) + -Ey * abs(Pny) + -Ez * abs(Pnz).
	// In our case we take as a rule that Ex = Ey = Ez. Since our tree is loose, we double all extents.
	// Extents of tree nodes are obtained by multiplying this by a node size coefficient.
	const auto NegWorldExtent4 = acl::vector_set(-2.f * _WorldExtent);
	auto ProjectedNegWorldExtent = acl::vector_mul(NegWorldExtent4, acl::vector_abs(LRBT_Nx));
	ProjectedNegWorldExtent = acl::vector_mul_add(NegWorldExtent4, acl::vector_abs(LRBT_Ny), ProjectedNegWorldExtent);
	ProjectedNegWorldExtent = acl::vector_mul_add(NegWorldExtent4, acl::vector_abs(LRBT_Nz), ProjectedNegWorldExtent);

	// Near and far planes are parallel, which enables us to save some work. Near & far planes are +d (-w) along the look
	// axis, so NearPlane is negated once and FarPlane is negated twice (once to make it -w, once to invert the axis).
	// We use D3D style projection matrix, near Z limit is 0 instead of OpenGL's -W.
	// TODO: check if it is really faster than making NF_Nx etc. At least less registers used? Also could use AVX to handle all 6 planes at once.
	const auto NearAxis = acl::vector_set(ViewProj.m[0][2], ViewProj.m[1][2], ViewProj.m[2][2], 0.f);
	const auto FarAxis = acl::vector_sub(acl::vector_set(ViewProj.m[0][3], ViewProj.m[1][3], ViewProj.m[2][3], 0.f), NearAxis);
	const float InvNearLen = acl::sqrt_reciprocal(acl::vector_length_squared3(NearAxis));
	const auto LookAxis = acl::vector_mul(NearAxis, InvNearLen);
	const float NearPlane = -ViewProj.m[3][2] * InvNearLen;
	const float FarPlane = (ViewProj.m[3][3] - ViewProj.m[3][2]) * acl::sqrt_reciprocal(acl::vector_length_squared3(FarAxis));
	const float NegWorldExtentAlongLookAxis = acl::vector_dot3(acl::vector_abs(LookAxis), NegWorldExtent4);

	// Process the root outside the loop to simplify conditions inside
	// FIXME: improve writing, clip mask already has both bits for NodeVisibility element
	const auto WorldBounds = acl::vector_set(_WorldCenter.x, _WorldCenter.y, _WorldCenter.z, 1.f);
	const auto ClipRoot = ClipCube(WorldBounds, ProjectedNegWorldExtent, LRBT_Nx, LRBT_Ny, LRBT_Nz, LRBT_w, LookAxis, NegWorldExtentAlongLookAxis, NearPlane, FarPlane);
	NodeVisibility[0] = ClipRoot & EClipStatus::Inside;
	NodeVisibility[1] = ClipRoot & EClipStatus::Outside;

	// Skip the root as it is already processed
	for (auto ItNode = ++_TreeNodes.cbegin(); ItNode != _TreeNodes.cend(); ++ItNode)
	{
		//!!!DBG TMP! CSparseArray2 guarantees the order, but we check twice.
		n_assert_dbg(ItNode->ParentIndex < ItNode.get_index());

		// FIXME: what if node is removed and another node is added at the same index. Cache needs to be invalidated. Skip caching for now.
		//// Don't update nodes cached from previous frames
		//if (ItNode.get_index() < CachedCount && (NodeVisibility[ItNode.get_index() * 2] || NodeVisibility[ItNode.get_index() * 2 + 1]))
		//	continue;

		const bool IsParentInside = NodeVisibility[ItNode->ParentIndex * 2];
		const bool IsParentOutside = NodeVisibility[ItNode->ParentIndex * 2 + 1];
		if (IsParentInside != IsParentOutside)
		{
			// If the parent is completely visible or completely invisible, all its children have the same state
			// FIXME: improve writing, parent state can be read as 2 bits mask in one(?) op
			NodeVisibility[ItNode.get_index() * 2] = IsParentInside;
			NodeVisibility[ItNode.get_index() * 2 + 1] = IsParentOutside;
		}
		else
		{
			// If the parent is partially visible, its children must be tested
			// FIXME: improve writing, clip mask already has both bits for NodeVisibility element
			const auto ClipNode = ClipCube(ItNode->Bounds, ProjectedNegWorldExtent, LRBT_Nx, LRBT_Ny, LRBT_Nz, LRBT_w, LookAxis, NegWorldExtentAlongLookAxis, NearPlane, FarPlane);
			NodeVisibility[ItNode.get_index() * 2] = ClipNode & EClipStatus::Inside;
			NodeVisibility[ItNode.get_index() * 2 + 1] = ClipNode & EClipStatus::Outside;
		}
	}
}
//---------------------------------------------------------------------

}
