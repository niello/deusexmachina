#include "GraphicsScene.h"
#include <Frame/RenderableAttribute.h> // for casting to CNodeAttribute
#include <Frame/LightAttribute.h> // for casting to CNodeAttribute
#include <Math/Math.h>
#include <Math/CameraMath.h>

namespace Frame
{

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
	// Depth = GetDepthLevel(Morton);
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

void CGraphicsScene::Init(const vector3& Center, float Size, U8 HierarchyDepth)
{
	n_assert2_dbg(Size >= 0.f, "CGraphicsScene::Init() > negative world extent is not allowed!");

	_MaxDepth = std::min(HierarchyDepth, TREE_MAX_DEPTH);
	_WorldExtent = Size * 0.5f;
	_InvWorldSize = 1.f / Size;
	_SmallestExtent = _WorldExtent / static_cast<float>(1 << _MaxDepth);

	// Create a root node. This simplifies object insertion logic.
	// Set object count to fake 1 to keep the root alive forever.
	auto& Root = *_TreeNodes.emplace();
	Root.Bounds = acl::vector_set(Center.x, Center.y, Center.z, 1.f);
	Root.MortonCode = 1;
	Root.ParentIndex = NO_SPATIAL_TREE_NODE;
	Root.SubtreeObjectCount = 1;
	_MortonToIndex.emplace(1, 0);
}
//---------------------------------------------------------------------

TMorton CGraphicsScene::CalculateMortonCode(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent) const noexcept
{
	const auto WorldCenter = _TreeNodes[0].Bounds;
	const auto WorldExtent = acl::vector_set(_WorldExtent);

	// Check for location outside the world bounds. Loose tree requires only the center being inside.
	const auto CenterDiff = acl::vector_abs(acl::vector_sub(BoxCenter, WorldCenter));
	if (acl::vector_any_greater_equal3(CenterDiff, WorldExtent)) return 0;

	// Our level is where the non-loose node size is not less than our size in any of dimensions.
	// Too small and degenerate AABBs sink to the deepest possible level to save us from division errors.
	TMorton NodeSizeCoeff = static_cast<TMorton>(1 << _MaxDepth);
	if (acl::vector_any_greater_equal3(BoxExtent, acl::vector_set(_SmallestExtent)))
	{
		// TODO: can make better with SIMD?
		const float MaxDim = std::max({ acl::vector_get_x(BoxExtent), acl::vector_get_y(BoxExtent), acl::vector_get_z(BoxExtent) });
		const TMorton HighestSharePow2 = Math::PrevPow2(static_cast<TMorton>(_WorldExtent / MaxDim));
		if (NodeSizeCoeff > HighestSharePow2) NodeSizeCoeff = HighestSharePow2;
	}

	const float CellCoeff = static_cast<float>(NodeSizeCoeff) * _InvWorldSize;
	const auto Cell = acl::vector_mul(acl::vector_add(BoxCenter, acl::vector_sub(WorldExtent, WorldCenter)), CellCoeff); // (C + (We - Wc)) * Coeff

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

U32 CGraphicsScene::CreateNode(U32 FreeIndex, TMorton MortonCode, U32 ParentIndex)
{
	// Create a node
	auto ItNew = _TreeNodes.emplace_at_free(FreeIndex);
	ItNew->MortonCode = MortonCode;
	ItNew->ParentIndex = ParentIndex;
	ItNew->SubtreeObjectCount = 1;
	ItNew->Bounds = CalcNodeBounds(MortonCode);

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

U32 CGraphicsScene::AddSingleObjectToNode(TMorton NodeMortonCode, TMorton StopMortonCode)
{
	if (!NodeMortonCode) return NO_SPATIAL_TREE_NODE;

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
	if (FreeIndex != decltype(_TreeNodes)::INVALID_INDEX)
		_SpatialTreeRebuildVersion = std::max<U32>(1, _SpatialTreeRebuildVersion + 1); // Existing nodes changed
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

void CGraphicsScene::RemoveSingleObjectFromNode(U32 NodeIndex, TMorton NodeMortonCode, TMorton StopMortonCode)
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

CGraphicsScene::HRecord CGraphicsScene::AddObject(std::map<UPTR, CSpatialRecord>& Storage, UPTR UID, const CAABB& GlobalBox, Scene::CNodeAttribute& Attr)
{
	CSpatialRecord Record;
	Record.pAttr = &Attr;

	// TODO: store AABB as SIMD center-extents everywhere?!
	const auto HalfMin = acl::vector_mul(acl::vector_set(GlobalBox.Min.x, GlobalBox.Min.y, GlobalBox.Min.z), 0.5f);
	const auto HalfMax = acl::vector_mul(acl::vector_set(GlobalBox.Max.x, GlobalBox.Max.y, GlobalBox.Max.z), 0.5f);
	Record.BoxCenter = acl::vector_add(HalfMax, HalfMin);
	Record.BoxExtent = acl::vector_sub(HalfMax, HalfMin);

	// FIXME: get from outside, as GlobalBox, it will be more optimal, potentially much less space wasted!
	// RTCD 4.3.2 Computing a Bounding Sphere
	Record.SphereRadius = acl::vector_length3(Record.BoxExtent);

	// Check bounds validity
	if (!acl::vector_any_less_than3(Record.BoxExtent, acl::vector_set(0.f))) //!!!TODO: can check negative sign bits by mask!
	{
		const auto NodeMortonCode = CalculateMortonCode(Record.BoxCenter, Record.BoxExtent);
		Record.NodeIndex = AddSingleObjectToNode(NodeMortonCode, 0);
		Record.NodeMortonCode = NodeMortonCode;
		Record.BoundsVersion = 1;
	}
	else
	{
		// Objects with invalid bounds are considered being outside the spatial tree
		Record.NodeIndex = NO_SPATIAL_TREE_NODE;
		Record.NodeMortonCode = 0;
		Record.BoundsVersion = 0;
	}

	if (_ObjectNodePool.empty())
	{
		return Storage.emplace_hint(Storage.cend(), UID, std::move(Record));
	}
	else
	{
		auto Node = std::move(_ObjectNodePool.back());
		_ObjectNodePool.pop_back();
		Node.key() = UID;
		new (&Node.mapped()) CSpatialRecord(std::move(Record));
		return Storage.insert(Storage.cend(), std::move(Node));
	}
}
//---------------------------------------------------------------------

void CGraphicsScene::UpdateObjectBounds(HRecord Handle, const CAABB& GlobalBox)
{
	auto& Record = Handle->second;

	// TODO: store AABB as SIMD center-extents everywhere?!
	const auto HalfMin = acl::vector_mul(acl::vector_set(GlobalBox.Min.x, GlobalBox.Min.y, GlobalBox.Min.z), 0.5f);
	const auto HalfMax = acl::vector_mul(acl::vector_set(GlobalBox.Max.x, GlobalBox.Max.y, GlobalBox.Max.z), 0.5f);
	const auto BoxCenter = acl::vector_add(HalfMax, HalfMin);
	const auto BoxExtent = acl::vector_sub(HalfMax, HalfMin);

	// TODO PERF: check if this is useful. Also may want to rewrite for strict equality because near_equal involves much more operations!
	if (acl::vector_all_near_equal3(BoxCenter, Record.BoxCenter) && acl::vector_all_near_equal3(BoxExtent, Record.BoxExtent))
		return;

	const bool BoundsValid = !acl::vector_any_less_than3(Record.BoxExtent, acl::vector_set(0.f)); //!!!TODO: can check negative sign bits by mask!

	Record.BoxCenter = BoxCenter;
	Record.BoxExtent = BoxExtent;
	Record.BoundsVersion = BoundsValid ? std::max<U32>(1, Record.BoundsVersion + 1) : 0;

	// FIXME: get from outside, as GlobalBox, it will be more optimal, potentially much less space wasted!
	// RTCD 4.3.2 Computing a Bounding Sphere
	Record.SphereRadius = acl::vector_length3(Record.BoxExtent);

	const auto NodeMortonCode = BoundsValid ? CalculateMortonCode(Record.BoxCenter, Record.BoxExtent) : 0;
	if (Record.NodeMortonCode != NodeMortonCode)
	{
		const auto LCAMortonCode = Math::MortonLCA<TREE_DIMENSIONS>(Record.NodeMortonCode, NodeMortonCode);
		RemoveSingleObjectFromNode(Record.NodeIndex, Record.NodeMortonCode, LCAMortonCode);
		Record.NodeIndex = AddSingleObjectToNode(NodeMortonCode, LCAMortonCode);
		Record.NodeMortonCode = NodeMortonCode;
	}
}
//---------------------------------------------------------------------

// TODO: check safety. If causes issues, can use UID instead of an iterator, but this makes erase logarithmic instead of constant.
void CGraphicsScene::RemoveObject(std::map<UPTR, CSpatialRecord>& Storage, HRecord Handle)
{
	//if (Handle == Storage.cend()) return;

	//!!!TODO: erase linked list of intersections in which this object participates!
	//!!!for renderables and lights this is different code, need to move to RemoveRenderable and RemoveLight!

	RemoveSingleObjectFromNode(Handle->second.NodeIndex, Handle->second.NodeMortonCode, 0);
	_ObjectNodePool.push_back(Storage.extract(Handle));
	_ObjectNodePool.back().mapped().~CSpatialRecord();
}
//---------------------------------------------------------------------

CGraphicsScene::HRecord CGraphicsScene::AddRenderable(const CAABB& GlobalBox, CRenderableAttribute& RenderableAttr)
{
	const auto UID = _NextRenderableUID++;

	// If this assert is ever triggered, compacting of existing UIDs may be implemented to keep fast insertions to the map end.
	// Compacting must change UIDs in map keys and broadcast changes to all views. Try to make it sorted and preserve iterators to avoid logN searches.
	n_assert_dbg(_NextRenderableUID < std::numeric_limits<decltype(_NextRenderableUID)>().max());

	return AddObject(_Renderables, UID, GlobalBox, RenderableAttr);
}
//---------------------------------------------------------------------

CGraphicsScene::HRecord CGraphicsScene::AddLight(const CAABB& GlobalBox, CLightAttribute& LightAttr)
{
	const auto UID = _NextLightUID++;

	// If this assert is ever triggered, compacting of existing UIDs may be implemented to keep fast insertions to the map end.
	// Compacting must change UIDs in map keys and broadcast changes to all views. Try to make it sorted and preserve iterators to avoid logN searches.
	n_assert_dbg(_NextLightUID < std::numeric_limits<decltype(_NextLightUID)>().max());

	return AddObject(_Lights, UID, GlobalBox, LightAttr);
}
//---------------------------------------------------------------------

// Test AABB cube vs frustum planes containment or intersection for positive halfspace treated as 'inside'.
// Returns a 2 bit mask with bit0 set if the cube is present inside and bit1 set if the cube is present outside.
// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
static DEM_FORCE_INLINE U8 ClipCube(acl::Vector4_32Arg0 Bounds, acl::Vector4_32Arg1 ProjectedNegWorldExtent,
	float NegWorldExtentAlongLookAxis, const Math::CSIMDFrustum& Frustum) noexcept
{
	// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = acl::vector_mul_add(acl::vector_mix_xxxx(Bounds), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_yyyy(Bounds), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_zzzz(Bounds), Frustum.LRBT_Nz, CenterDistance);

	// Projection radius of the most outside vertex (-r)
	const auto ProjectedNegExtent = acl::vector_mul(ProjectedNegWorldExtent, acl::vector_mix_wwww(Bounds));

	// Check intersection with LRTB planes
	bool HasVisiblePart = acl::vector_all_greater_equal(CenterDistance, ProjectedNegExtent); //!!!need strictly greater (RTM?)!
	bool HasInvisiblePart = false;
	if (HasVisiblePart)
	{
		// If inside LRTB, check intersection with NF planes
		const float CenterAlongLookAxis = acl::vector_dot3(Frustum.LookAxis, Bounds);
		const float NegExtentAlongLookAxis = NegWorldExtentAlongLookAxis * acl::vector_get_w(Bounds);
		const float ClosestPoint = CenterAlongLookAxis + NegExtentAlongLookAxis;
		const float FarthestPoint = CenterAlongLookAxis - NegExtentAlongLookAxis;
		HasVisiblePart = (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
		HasInvisiblePart = !HasVisiblePart || (FarthestPoint > Frustum.FarPlane || ClosestPoint < Frustum.NearPlane);
	}

	HasInvisiblePart = HasInvisiblePart || acl::vector_any_less_than(CenterDistance, acl::vector_neg(ProjectedNegExtent));

	return static_cast<U8>(HasVisiblePart) | (static_cast<U8>(HasInvisiblePart) << 1);
}
//---------------------------------------------------------------------

// See Real-Time Collision Detection 5.2.3
// See https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/
void CGraphicsScene::TestSpatialTreeVisibility(const Math::CSIMDFrustum& Frustum, std::vector<bool>& NodeVisibility) const
{
	n_assert2_dbg(!_TreeNodes.empty(), "CGraphicsScene::TestSpatialTreeVisibility() should not be called before CGraphicsScene::Init()!");

	const size_t CachedCount = NodeVisibility.size() / 2;

	// Cell can have 4 frustum culling states, for which 2 bits are enough. And we use clever meanings for
	// individual bits inspired by UE, 'is inside' and 'is outside', so that we have the next states:
	// not checked (00), completely inside (01), completely outside (10), partially inside (11).
	NodeVisibility.resize(_TreeNodes.sparse_size() * 2, false);

	// Projection radius of the most outside vertex (-r): -Ex * abs(Pnx) + -Ey * abs(Pny) + -Ez * abs(Pnz).
	// In our case we take as a rule that Ex = Ey = Ez. Since our tree is loose, we double all extents.
	// Extents of tree nodes are obtained by multiplying this by a node size coefficient.
	const auto NegWorldExtent4 = acl::vector_set(-2.f * _WorldExtent);
	auto ProjectedNegWorldExtent = acl::vector_mul(NegWorldExtent4, acl::vector_abs(Frustum.LRBT_Nx));
	ProjectedNegWorldExtent = acl::vector_mul_add(NegWorldExtent4, acl::vector_abs(Frustum.LRBT_Ny), ProjectedNegWorldExtent);
	ProjectedNegWorldExtent = acl::vector_mul_add(NegWorldExtent4, acl::vector_abs(Frustum.LRBT_Nz), ProjectedNegWorldExtent);

	const float NegWorldExtentAlongLookAxis = acl::vector_dot3(acl::vector_abs(Frustum.LookAxis), NegWorldExtent4);

	// Skip cached nodes. For CachedCount == 0 const_iterator_at is equal to cbegin.
	auto ItNode = _TreeNodes.const_iterator_at(CachedCount);
	if (!CachedCount)
	{
		// Process the root outside the loop to simplify conditions inside
		// FIXME: improve writing, clip mask already has both bits for NodeVisibility element
		const auto ClipRoot = ClipCube(_TreeNodes[0].Bounds, ProjectedNegWorldExtent, NegWorldExtentAlongLookAxis, Frustum);
		NodeVisibility[0] = ClipRoot & EClipStatus::Inside;
		NodeVisibility[1] = ClipRoot & EClipStatus::Outside;

		// Skip the root as it is already processed
		++ItNode;
	}

	for (; ItNode != _TreeNodes.cend(); ++ItNode)
	{
		//!!!DBG TMP! CSparseArray2 guarantees the order, but we check twice.
		n_assert_dbg(ItNode->ParentIndex < ItNode.get_index());

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
			const auto ClipNode = ClipCube(ItNode->Bounds, ProjectedNegWorldExtent, NegWorldExtentAlongLookAxis, Frustum);
			NodeVisibility[ItNode.get_index() * 2] = ClipNode & EClipStatus::Inside;
			NodeVisibility[ItNode.get_index() * 2 + 1] = ClipNode & EClipStatus::Outside;
		}
	}
}
//---------------------------------------------------------------------

acl::Vector4_32 CGraphicsScene::CalcNodeBounds(TMorton MortonCode) const
{
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
	Center = acl::vector_mul_add(Center, _WorldExtent, _TreeNodes[0].Bounds); // Center = B * We + Wc

	// Set extent coeff to W
	return acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::A>(Center, acl::vector_set(ExtentCoeff));
}
//---------------------------------------------------------------------

CAABB CGraphicsScene::GetNodeAABB(U32 NodeIndex, bool Loose) const
{
	CAABB Box;
	if (NodeIndex != NO_SPATIAL_TREE_NODE)
		Box = GetNodeAABB(_TreeNodes[NodeIndex].Bounds, Loose);
	return Box;
}
//---------------------------------------------------------------------

CAABB CGraphicsScene::GetNodeAABB(acl::Vector4_32Arg0 Bounds, bool Loose) const
{
	const vector3 Center(acl::vector_get_x(Bounds), acl::vector_get_y(Bounds), acl::vector_get_z(Bounds));
	const float Extent = _WorldExtent * acl::vector_get_w(Bounds) * (Loose ? 2.f : 1.f);
	return CAABB(Center, vector3(Extent, Extent, Extent));
}
//---------------------------------------------------------------------

void CGraphicsScene::TrackObjectLightIntersections(CRenderableAttribute& RenderableAttr, bool Track)
{
	auto& Record = RenderableAttr.GetSceneHandle()->second;
	if (Track)
	{
		// TODO: if (!Record.TrackObjectLightIntersections) insert to acceleration structure (octree?)

		n_assert_dbg(Record.TrackObjectLightIntersections < std::numeric_limits<U8>().max());
		if (Record.TrackObjectLightIntersections < std::numeric_limits<U8>().max())
			++Record.TrackObjectLightIntersections;
	}
	else
	{
		// NB: we don't erase existing intersections because they may remain valid even when tracking is disabled (e.g. due to visibility change)
		n_assert_dbg(Record.TrackObjectLightIntersections);
		if (Record.TrackObjectLightIntersections)
			--Record.TrackObjectLightIntersections;

		// TODO: if (!Record.TrackObjectLightIntersections), remove from acceleration structure (octree?)
	}
}
//---------------------------------------------------------------------

void CGraphicsScene::TrackObjectLightIntersections(CLightAttribute& LightAttr, bool Track)
{
}
//---------------------------------------------------------------------

void CGraphicsScene::UpdateObjectLightIntersections(CRenderableAttribute& RenderableAttr)
{
	auto& RenderableRecord = RenderableAttr.GetSceneHandle()->second;
	n_assert_dbg(RenderableRecord.TrackObjectLightIntersections && RenderableRecord.BoundsVersion);

	// Indicate the last update time
	RenderableRecord.IntersectionBoundsVersion = RenderableRecord.BoundsVersion;

	// TODO: instead of iterating through all _Lights, can query them from octree! Will be much less instances to check!!!
	// Query by our attr's bounds and octree node. Go to all parents of the node and to only intersecting children, see UE:
	// - FindElementsWithBoundsTestInternal, GetIntersectingChildren
	//???how to handle not sorted collection, e.g. data from octree query?
	//???query to vector (stored in class to avoid per frame allocations) and sort pefore processing? Will O(query+sort) be better than O(bruteforce)?
	//???how UE avoids recreating already existing links? can't find. does it erase the proxy and then refill links when something changes?

	// An adapted version of DEM::Algo::SortedUnion for syncing with CObjectLightIntersection. Both collections are sorted by UID.
	// The logic is simplified in comparison with original SortedUnion because we guarantee that no intersections exist for removed objects and lights.
	auto pSyncIntersection = RenderableRecord.pObjectLightIntersections;
	for (auto& [UID, LightRecord] : _Lights)
	{
		CObjectLightIntersection* pMatchingIntersection;
		if (!pSyncIntersection || UID < pSyncIntersection->pLightAttr->GetSceneHandle()->first)
		{
			pMatchingIntersection = nullptr;
		}
		else // equal UIDs, matching intersection found
		{
			n_assert_dbg(pSyncIntersection && pSyncIntersection->pLightAttr->GetSceneHandle()->first == UID);

			pMatchingIntersection = pSyncIntersection;
			pSyncIntersection = pSyncIntersection->pNextLight;

			// Skip if has up to date intersection (e.g. when both renderable and light get updated at the same frame)
			if (pMatchingIntersection->RenderableBoundsVersion == RenderableRecord.BoundsVersion) continue;
		}

		// Don't erase existing connection in this case, tracking may be re-enabled later and connection may remain valid
		//!!!FIXME: if querying octree, there won't be objects with zero TrackObjectLightIntersections, so this condition won't be needed!
		if (!LightRecord.TrackObjectLightIntersections) continue;

		// Only local lights (ones with BoundsVersion > 0) track intersections
		//???can optimize intersection for terrain? could use AABB tree as an additional pass of light culling. Or disable tracking for terrain and override manually?
		auto pLightAttr = static_cast<CLightAttribute*>(LightRecord.pAttr);
		const bool Intersects = LightRecord.BoundsVersion && pLightAttr->IntersectsWith(RenderableRecord.BoxCenter, RenderableRecord.SphereRadius);
		if (Intersects)
		{
			if (pMatchingIntersection)
			{
				//!!!DBG TMP! Both are just to check if some of assignments is always redundant (I'm too lazy, I know)
				n_assert_dbg(pMatchingIntersection->LightBoundsVersion == LightRecord.BoundsVersion);
				n_assert_dbg(pMatchingIntersection->RenderableBoundsVersion == RenderableRecord.BoundsVersion);

				pMatchingIntersection->LightBoundsVersion = LightRecord.BoundsVersion;
				pMatchingIntersection->RenderableBoundsVersion = RenderableRecord.BoundsVersion;
			}
			else
			{
				auto pIntersection = _IntersectionPool.Construct();
				pIntersection->pRenderableAttr = static_cast<CRenderableAttribute*>(RenderableRecord.pAttr);
				pIntersection->pLightAttr = pLightAttr;
				pIntersection->LightBoundsVersion = LightRecord.BoundsVersion;
				pIntersection->RenderableBoundsVersion = RenderableRecord.BoundsVersion;

				// ... insert intersection to lists, handle sync iterator correctly, if used ...
				//???preserve sorting in both lists for perfect sync?
				// ... update intersection state version for renderable (and maybe for light) ...
			}
		}
		else if (!Intersects && pMatchingIntersection)
		{
			// ... remove intersection from lists, advance iterator if used for syncing ...
			// ... update intersection state version for renderable (and maybe for light) ...

			_IntersectionPool.Destroy(pMatchingIntersection);
		}
	}
}
//---------------------------------------------------------------------

void CGraphicsScene::UpdateObjectLightIntersections(CLightAttribute& LightAttr)
{
}
//---------------------------------------------------------------------

}
