#include "GraphicsScene.h"
#include <Frame/RenderableAttribute.h> // for casting to CNodeAttribute
#include <Frame/LightAttribute.h> // for casting to CNodeAttribute
#include <Math/Math.h>
#include <Math/CameraMath.h>
#include <Util/Utils.h>

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

static inline rtm::vector4f SphereFromBox(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent)
{
	const float SphereRadius = rtm::vector_length3(BoxExtent);
	return rtm::vector_set(rtm::vector_get_x(BoxCenter), rtm::vector_get_y(BoxCenter), rtm::vector_get_z(BoxCenter), SphereRadius);
}
//---------------------------------------------------------------------

static inline void AttachObjectLightIntersection(CObjectLightIntersection* pIntersection,
	CObjectLightIntersection** ppLightSlot, CObjectLightIntersection** ppRenderableSlot)
{
	// Insert to the light list preserving sorted order
	const auto pNextLight = *ppLightSlot;
	*ppLightSlot = pIntersection;
	pIntersection->ppPrevLightNext = ppLightSlot;
	pIntersection->pNextLight = pNextLight;
	if (pNextLight) pNextLight->ppPrevLightNext = &pIntersection->pNextLight;

	// Insert to the renderable list of this light
	const auto pNextRenderable = *ppRenderableSlot;
	*ppRenderableSlot = pIntersection;
	pIntersection->ppPrevRenderableNext = ppRenderableSlot;
	pIntersection->pNextRenderable = pNextRenderable;
	if (pNextRenderable) pNextRenderable->ppPrevRenderableNext = &pIntersection->pNextRenderable;
}
//---------------------------------------------------------------------

static inline void DetachObjectLightIntersection(CObjectLightIntersection* pIntersection)
{
	// Remove from the light list
	auto pNextLight = pIntersection->pNextLight;
	if (pNextLight) pNextLight->ppPrevLightNext = pIntersection->ppPrevLightNext;
	*pIntersection->ppPrevLightNext = pNextLight;

	// Remove from the renderable list of this light
	auto pNextRenderable = pIntersection->pNextRenderable;
	if (pNextRenderable) pNextRenderable->ppPrevRenderableNext = pIntersection->ppPrevRenderableNext;
	*pIntersection->ppPrevRenderableNext = pNextRenderable;
}
//---------------------------------------------------------------------

void CGraphicsScene::Init(const rtm::vector4f& Center, float Size, U8 HierarchyDepth)
{
	n_assert2_dbg(Size >= 0.f, "CGraphicsScene::Init() > negative world extent is not allowed!");

	_MaxDepth = std::min(HierarchyDepth, TREE_MAX_DEPTH);
	_WorldExtent = Size * 0.5f;
	_InvWorldSize = 1.f / Size;
	_SmallestExtent = _WorldExtent / static_cast<float>(1 << _MaxDepth);

	// Create a root node. This simplifies object insertion logic.
	// Set object count to fake 1 to keep the root alive forever.
	auto& Root = *_TreeNodes.emplace();
	Root.Bounds = rtm::vector_set_w(Center, 1.f);
	Root.MortonCode = 1;
	Root.ParentIndex = NO_SPATIAL_TREE_NODE;
	Root.SubtreeObjectCount = 1;
	_MortonToIndex.emplace(1, 0);
}
//---------------------------------------------------------------------

// O(1) calculation that exploits properties of the loose octree
TSceneMorton CGraphicsScene::CalculateMortonCode(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const noexcept
{
	const auto WorldCenter = _TreeNodes[0].Bounds;
	const auto WorldExtent = rtm::vector_set(_WorldExtent);

	// Check for location outside the world bounds. Loose tree requires only the center being inside.
	const auto CenterDiff = rtm::vector_abs(rtm::vector_sub(BoxCenter, WorldCenter));
	if (rtm::vector_any_greater_equal3(CenterDiff, WorldExtent)) return 0;

	// Our level is where the non-loose node size is not less than our size in any of dimensions.
	// Too small and degenerate AABBs sink to the deepest possible level to save us from division errors.
	TSceneMorton NodeSizeCoeff = static_cast<TSceneMorton>(1 << _MaxDepth);
	if (rtm::vector_any_greater_equal3(BoxExtent, rtm::vector_set(_SmallestExtent)))
	{
		// TODO: can make better horizontal max() with SIMD?
		const float MaxDim = std::max<float>({ rtm::vector_get_x(BoxExtent), rtm::vector_get_y(BoxExtent), rtm::vector_get_z(BoxExtent) });
		const TSceneMorton HighestSharePow2 = Math::PrevPow2(static_cast<TSceneMorton>(_WorldExtent / MaxDim));
		if (NodeSizeCoeff > HighestSharePow2) NodeSizeCoeff = HighestSharePow2;
	}

	const float CellCoeff = static_cast<float>(NodeSizeCoeff) * _InvWorldSize;
	const auto Cell = rtm::vector_mul(rtm::vector_add(BoxCenter, rtm::vector_sub(WorldExtent, WorldCenter)), CellCoeff); // (C + (We - Wc)) * Coeff

	const auto x = static_cast<TSceneCellDim>(rtm::vector_get_x(Cell));
	const auto y = static_cast<TSceneCellDim>(rtm::vector_get_y(Cell));
	const auto z = static_cast<TSceneCellDim>(rtm::vector_get_z(Cell));

	// NodeSizeCoeff bit is offset by Depth bits. Its pow(N) is a bit offset to N*Depth, making a room for N-dimensional Morton code.
	if constexpr (TREE_DIMENSIONS == 2)
		return (NodeSizeCoeff * NodeSizeCoeff) | Math::MortonCode2(x, z);
	else if constexpr (sizeof(TSceneMorton) >= 8)
		return (NodeSizeCoeff * NodeSizeCoeff * NodeSizeCoeff) | Math::MortonCode3_21bit(x, y, z);
	else
		return (NodeSizeCoeff * NodeSizeCoeff * NodeSizeCoeff) | Math::MortonCode3_10bit(x, y, z);
}
//---------------------------------------------------------------------

U32 CGraphicsScene::CreateNode(U32 FreeIndex, TSceneMorton MortonCode, U32 ParentIndex)
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

U32 CGraphicsScene::AddSingleObjectToNode(TSceneMorton NodeMortonCode, TSceneMorton StopMortonCode)
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
		IncrementVersion(_SpatialTreeRebuildVersion); // Existing nodes changed
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

void CGraphicsScene::RemoveSingleObjectFromNode(U32 NodeIndex, TSceneMorton NodeMortonCode, TSceneMorton StopMortonCode)
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

CGraphicsScene::HRecord CGraphicsScene::AddObject(std::map<UPTR, CSpatialRecord>& Storage, UPTR UID,
	const Math::CAABB& GlobalBox, rtm::vector4f_arg0 GlobalSphere, Scene::CNodeAttribute& Attr)
{
	CSpatialRecord Record;
	Record.pAttr = &Attr;
	Record.Box = GlobalBox;
	Record.Sphere = GlobalSphere;

	// Check bounds validity
	if (Math::IsAABBValid(Record.Box))
	{
		const auto NodeMortonCode = CalculateMortonCode(Record.Box.Center, Record.Box.Extent);
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

void CGraphicsScene::UpdateObjectBounds(HRecord Handle, const Math::CAABB& GlobalBox, rtm::vector4f_arg0 GlobalSphere)
{
	auto& Record = Handle->second;

	// TODO PERF: check if this is useful
	if (rtm::vector_all_equal3(GlobalBox.Center, Record.Box.Center) && rtm::vector_all_equal3(GlobalBox.Extent, Record.Box.Extent))
		return;

	const bool BoundsValid = Math::IsAABBValid(Record.Box);

	Record.Box = GlobalBox;
	Record.Sphere = GlobalSphere;
	if (BoundsValid)
		IncrementVersion(Record.BoundsVersion);
	else
		Record.BoundsVersion = 0;

	const auto NodeMortonCode = BoundsValid ? CalculateMortonCode(Record.Box.Center, Record.Box.Extent) : 0;
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
	RemoveSingleObjectFromNode(Handle->second.NodeIndex, Handle->second.NodeMortonCode, 0);
	_ObjectNodePool.push_back(Storage.extract(Handle));
	_ObjectNodePool.back().mapped().~CSpatialRecord();
}
//---------------------------------------------------------------------

CGraphicsScene::HRecord CGraphicsScene::AddRenderable(const Math::CAABB& GlobalBox, CRenderableAttribute& RenderableAttr)
{
	const auto UID = _NextRenderableUID++;

	// If this assert is ever triggered, compacting of existing UIDs may be implemented to keep fast insertions to the map end.
	// Compacting must change UIDs in map keys and broadcast changes to all views. Try to make it sorted and preserve iterators to avoid logN searches.
	n_assert_dbg(_NextRenderableUID < std::numeric_limits<decltype(_NextRenderableUID)>().max());

	// FIXME: precalculate in tools instead!
	// See RTCD Chapter 4.3.2: Computing a Bounding Sphere
	const auto Sphere = SphereFromBox(GlobalBox.Center, GlobalBox.Extent);

	return AddObject(_Renderables, UID, GlobalBox, Sphere, RenderableAttr);
}
//---------------------------------------------------------------------

void CGraphicsScene::UpdateRenderableBounds(HRecord Handle, const Math::CAABB& GlobalBox)
{
	// FIXME: precalculate in tools instead!
	// See RTCD Chapter 4.3.2: Computing a Bounding Sphere
	const auto Sphere = SphereFromBox(GlobalBox.Center, GlobalBox.Extent);

	UpdateObjectBounds(Handle, GlobalBox, Sphere);
}
//---------------------------------------------------------------------

void CGraphicsScene::RemoveRenderable(HRecord Handle)
{
	// TODO: if TrackObjectLightIntersections, remove from acceleration structure

	// Erase all intersections from the light list of this renderable
	auto pIntersection = Handle->second.pObjectLightIntersections;
	while (pIntersection)
	{
		// Remove from the renderable list of the current light
		auto pNextRenderable = pIntersection->pNextRenderable;
		if (pNextRenderable) pNextRenderable->ppPrevRenderableNext = pIntersection->ppPrevRenderableNext;
		*pIntersection->ppPrevRenderableNext = pNextRenderable;

		auto pNextIntersection = pIntersection->pNextLight;
		_IntersectionPool.Destroy(pIntersection);
		pIntersection = pNextIntersection;
	}

	RemoveObject(_Renderables, Handle);
}
//---------------------------------------------------------------------

CGraphicsScene::HRecord CGraphicsScene::AddLight(const Math::CAABB& GlobalBox, rtm::vector4f_arg0 GlobalSphere, CLightAttribute& LightAttr)
{
	const auto UID = _NextLightUID++;

	// If this assert is ever triggered, compacting of existing UIDs may be implemented to keep fast insertions to the map end.
	// Compacting must change UIDs in map keys and broadcast changes to all views. Try to make it sorted and preserve iterators to avoid logN searches.
	n_assert_dbg(_NextLightUID < std::numeric_limits<decltype(_NextLightUID)>().max());

	return AddObject(_Lights, UID, GlobalBox, GlobalSphere, LightAttr);
}
//---------------------------------------------------------------------

void CGraphicsScene::UpdateLightBounds(HRecord Handle, const Math::CAABB& GlobalBox, rtm::vector4f_arg0 GlobalSphere)
{
	UpdateObjectBounds(Handle, GlobalBox, GlobalSphere);
}
//---------------------------------------------------------------------

void CGraphicsScene::RemoveLight(HRecord Handle)
{
	// TODO: if TrackObjectLightIntersections, remove from acceleration structure (light quadtree or such)

	// Erase all intersections from the renderable list of this light
	auto pIntersection = Handle->second.pObjectLightIntersections;
	while (pIntersection)
	{
		// Remove from the light list of the current renderable
		auto pNextLight = pIntersection->pNextLight;
		if (pNextLight) pNextLight->ppPrevLightNext = pIntersection->ppPrevLightNext;
		*pIntersection->ppPrevLightNext = pNextLight;

		// Notify the current renderable that its light intersection list have changed
		if (pIntersection->pRenderableAttr->GetLightTrackingFlags() & CRenderableAttribute::TrackLightContactChanges)
			IncrementVersion(pIntersection->pRenderableAttr->GetSceneHandle()->second.ObjectLightIntersectionsVersion);

		auto pNextIntersection = pIntersection->pNextRenderable;
		_IntersectionPool.Destroy(pIntersection);
		pIntersection = pNextIntersection;
	}

	RemoveObject(_Lights, Handle);
}
//---------------------------------------------------------------------

// Test AABB cube vs frustum planes containment or intersection for positive halfspace treated as 'inside'.
// It is a variant of Math::ClipAABB optimized for very certain conditions.
// Bounds arg is (x, y, z) origin and w extent of the cube.
// Returns a 2 bit mask with bit0 set if the cube is present inside and bit1 set if the cube is present outside.
// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
static DEM_FORCE_INLINE U8 ClipCube(rtm::vector4f_arg0 Bounds, rtm::vector4f_arg1 ProjectedWorldExtent,
	float WorldExtentAlongLookAxis, const Math::CSIMDFrustum& Frustum) noexcept
{
	// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = rtm::vector_mul_add(rtm::vector_dup_x(Bounds), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = rtm::vector_mul_add(rtm::vector_dup_y(Bounds), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = rtm::vector_mul_add(rtm::vector_dup_z(Bounds), Frustum.LRBT_Nz, CenterDistance);

	// Projection radius of the most outside vertex (-r)
	const auto ProjectedExtent = rtm::vector_mul(ProjectedWorldExtent, rtm::vector_dup_w(Bounds));

	// Check intersection with LRTB planes
	bool HasVisiblePart = rtm::vector_all_less_equal(CenterDistance, ProjectedExtent);
	bool HasInvisiblePart = false;
	if (HasVisiblePart)
	{
		// If inside LRTB, check intersection with NF planes
		const float CenterAlongLookAxis = rtm::vector_dot3(Frustum.LookAxis, Bounds);
		const float ExtentAlongLookAxis = WorldExtentAlongLookAxis * rtm::vector_get_w(Bounds);
		const float ClosestPoint = CenterAlongLookAxis - ExtentAlongLookAxis;
		const float FarthestPoint = CenterAlongLookAxis + ExtentAlongLookAxis;
		HasVisiblePart = (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
		HasInvisiblePart = !HasVisiblePart || (FarthestPoint > Frustum.FarPlane || ClosestPoint < Frustum.NearPlane);
	}

	HasInvisiblePart = HasInvisiblePart || rtm::vector_any_greater_equal(CenterDistance, rtm::vector_neg(ProjectedExtent));

	return static_cast<U8>(HasVisiblePart) | (static_cast<U8>(HasInvisiblePart) << 1);
}
//---------------------------------------------------------------------

// See Real-Time Collision Detection 5.2.3
// See https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/
void CGraphicsScene::TestSpatialTreeVisibility(const Math::CSIMDFrustum& Frustum, std::vector<bool>& NodeVisibility) const
{
	ZoneScoped;

	n_assert2_dbg(!_TreeNodes.empty(), "CGraphicsScene::TestSpatialTreeVisibility() should not be called before CGraphicsScene::Init()!");

	const size_t CachedCount = NodeVisibility.size() / 2;

	// Cell can have 4 frustum culling states, for which 2 bits are enough. And we use clever meanings for
	// individual bits inspired by UE, 'is inside' and 'is outside', so that we have the next states:
	// not checked (00), completely inside (01), completely outside (10), partially inside (11).
	NodeVisibility.resize(_TreeNodes.sparse_size() * 2, false);

	// Projection radius of the most inside vertex (r): Ex * abs(Pnx) + Ey * abs(Pny) + Ez * abs(Pnz).
	// In our case we take as a rule that Ex = Ey = Ez. Since our tree is loose, we double all extents.
	// Extents of tree nodes are obtained by multiplying this by a node size coefficient.
	constexpr float LOOSE_BOUNDS_MULTIPLIER = 2.f;
	const auto WorldExtent4 = rtm::vector_set(LOOSE_BOUNDS_MULTIPLIER * _WorldExtent);
	auto ProjectedWorldExtent = rtm::vector_mul(WorldExtent4, rtm::vector_abs(Frustum.LRBT_Nx));
	ProjectedWorldExtent = rtm::vector_mul_add(WorldExtent4, rtm::vector_abs(Frustum.LRBT_Ny), ProjectedWorldExtent);
	ProjectedWorldExtent = rtm::vector_mul_add(WorldExtent4, rtm::vector_abs(Frustum.LRBT_Nz), ProjectedWorldExtent);

	const float WorldExtentAlongLookAxis = rtm::vector_dot3(rtm::vector_abs(Frustum.LookAxis), WorldExtent4);

	// Skip cached nodes. For CachedCount == 0 const_iterator_at is equal to cbegin.
	auto ItNode = _TreeNodes.const_iterator_at(CachedCount);
	if (!CachedCount)
	{
		// Process the root outside the loop to simplify conditions inside
		// FIXME: improve writing, clip mask already has both bits for NodeVisibility element
		const auto ClipRoot = ClipCube(_TreeNodes[0].Bounds, ProjectedWorldExtent, WorldExtentAlongLookAxis, Frustum);
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
			const auto ClipNode = ClipCube(ItNode->Bounds, ProjectedWorldExtent, WorldExtentAlongLookAxis, Frustum);
			NodeVisibility[ItNode.get_index() * 2] = ClipNode & EClipStatus::Inside;
			NodeVisibility[ItNode.get_index() * 2 + 1] = ClipNode & EClipStatus::Outside;
		}
	}
}
//---------------------------------------------------------------------

rtm::vector4f CGraphicsScene::CalcNodeBounds(TSceneMorton MortonCode) const
{
	// Unpack Morton code back into cell coords and depth
	const auto Bits = Math::BitWidth(MortonCode);
	const auto MortonCodeNoSentinel = MortonCode ^ (1 << (Bits - 1));
	TSceneCellDim x = 0, y = 0, z = 0;
	if constexpr (TREE_DIMENSIONS == 2)
		Math::MortonDecode2(MortonCodeNoSentinel, x, z);
	else
		Math::MortonDecode3(MortonCodeNoSentinel, x, y, z);

	// Calculate node bounds - center and extent
	const float ExtentCoeff = 1.f / static_cast<float>(1 << (Bits / TREE_DIMENSIONS)); // 1 / 2^Depth
	const auto Cell = rtm::vector_set(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	auto Center = rtm::vector_mul_add(Cell, 2.f, rtm::vector_set(1.f)); // A = 2 * xyz + 1
	Center = rtm::vector_mul_add(Center, ExtentCoeff, rtm::vector_set(-1.f)); // B = A * Ecoeff - 1.f
	Center = rtm::vector_mul_add(Center, _WorldExtent, _TreeNodes[0].Bounds); // Center = B * We + Wc

	// Set extent coeff to W
	return Math::vector_mix_xyza(Center, rtm::vector_set(ExtentCoeff));
}
//---------------------------------------------------------------------

Math::CAABB CGraphicsScene::GetNodeAABB(U32 NodeIndex, bool Loose) const
{
	Math::CAABB Box;
	if (NodeIndex != NO_SPATIAL_TREE_NODE)
	{
		Box = GetNodeAABB(_TreeNodes[NodeIndex].Bounds, Loose);
	}
	else
	{
		Box.Center = rtm::vector_zero();
		Box.Extent = rtm::vector_zero();
	}
	return Box;
}
//---------------------------------------------------------------------

Math::CAABB CGraphicsScene::GetNodeAABB(rtm::vector4f_arg0 Bounds, bool Loose) const
{
	const float Extent = _WorldExtent * rtm::vector_get_w(Bounds) * (Loose ? 2.f : 1.f);
	return Math::CAABB{ Bounds, rtm::vector_set(Extent) };
}
//---------------------------------------------------------------------

void CGraphicsScene::TrackObjectLightIntersections(CSpatialRecord& Record, bool Track)
{
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

void CGraphicsScene::TrackObjectLightIntersections(CRenderableAttribute& RenderableAttr, bool Track)
{
	TrackObjectLightIntersections(RenderableAttr.GetSceneHandle()->second, Track);
}
//---------------------------------------------------------------------

void CGraphicsScene::TrackObjectLightIntersections(CLightAttribute& LightAttr, bool Track)
{
	TrackObjectLightIntersections(LightAttr.GetSceneHandle()->second, Track);
}
//---------------------------------------------------------------------

void CGraphicsScene::UpdateObjectLightIntersections(CRenderableAttribute& RenderableAttr)
{
	const auto RenderableUID = RenderableAttr.GetSceneHandle()->first;
	auto& RenderableRecord = RenderableAttr.GetSceneHandle()->second;

	// Only light tracking objects with valid non-infinite bounds are expected here
	n_assert_dbg(RenderableRecord.TrackObjectLightIntersections && RenderableRecord.BoundsVersion);

	// Remember the last updated bounds, so that different views will not trigger redundant updates in a shared scene
	if (RenderableRecord.IntersectionBoundsVersion == RenderableRecord.BoundsVersion) return;
	RenderableRecord.IntersectionBoundsVersion = RenderableRecord.BoundsVersion;

	const auto TrackingFlags = static_cast<CRenderableAttribute*>(RenderableRecord.pAttr)->GetLightTrackingFlags();
	n_assert_dbg(TrackingFlags);

	// TODO: instead of iterating through all _Lights, can query them from octree! Will be much less instances to check!!!
	// Query by our attr's bounds and octree node. Go to all parents of the node and to only intersecting children, see UE:
	// - FindElementsWithBoundsTestInternal, GetIntersectingChildren
	//???how to handle not sorted collection, e.g. data from octree query?
	//???query to vector (stored in class to avoid per frame allocations) and sort pefore processing? Will O(query+sort) be better than O(bruteforce)?
	//???how UE avoids recreating already existing links? can't find. does it erase the proxy and then refill links when something changes?

	// An adapted version of DEM::Algo::SortedUnion for syncing with CObjectLightIntersection. Both collections are sorted by UID.
	// The logic is simplified in comparison with original SortedUnion because we guarantee that no intersections exist for removed objects and lights.
	CObjectLightIntersection** ppLightInsertionSlot = &RenderableRecord.pObjectLightIntersections;
	for (auto& [LightUID, LightRecord] : _Lights)
	{
		CObjectLightIntersection* pMatchingIntersection;
		if (!(*ppLightInsertionSlot) || LightUID < (*ppLightInsertionSlot)->pLightAttr->GetSceneHandle()->first)
		{
			pMatchingIntersection = nullptr;
		}
		else // equal UIDs, matching intersection found
		{
			n_assert_dbg((*ppLightInsertionSlot) && (*ppLightInsertionSlot)->pLightAttr->GetSceneHandle()->first == LightUID);

			pMatchingIntersection = (*ppLightInsertionSlot);
			ppLightInsertionSlot = &pMatchingIntersection->pNextLight;

			// Skip if has up to date intersection (e.g. when both renderable and light get updated at the same frame)
			//???FIXME: need to check both renderable and light here? or updated light must be completely skipped?!
			if (pMatchingIntersection->RenderableBoundsVersion == RenderableRecord.BoundsVersion) continue;
		}

		// Don't erase existing connection in this case, tracking may be re-enabled later and connection may remain valid
		//!!!FIXME: if querying scene octree, there won't be objects with zero TrackObjectLightIntersections, so this condition won't be needed!
		if (!LightRecord.TrackObjectLightIntersections) continue;

		// TODO: can use RenderableRecord.NodeMortonCode & LightRecord.NodeMortonCode for early true/false?
		//!!!then the same needed for UpdateObjectLightIntersections(CLightAttribute& LightAttr)!

		// Only local lights (ones with BoundsVersion > 0) track intersections
		const bool Intersects = LightRecord.BoundsVersion && static_cast<CLightAttribute*>(LightRecord.pAttr)->IntersectsWith(RenderableRecord.Sphere);
		if (Intersects)
		{
			if (!pMatchingIntersection)
			{
				pMatchingIntersection = _IntersectionPool.Construct();
				pMatchingIntersection->pRenderableAttr = static_cast<CRenderableAttribute*>(RenderableRecord.pAttr);
				pMatchingIntersection->pLightAttr = static_cast<CLightAttribute*>(LightRecord.pAttr);

				// Find the position in the renderable list of this light to preserve UID sorting
				// TODO PERF: now O(n). If critical, can switch from linked list to std::map for O(logN), but anyway clustered lighting should reduce workload a lot.
				// Could break sorting and insert in O(1) like in UE, but then need to clear and refill intersections each frame and resubmit to GPU too.
				auto ppRenderableInsertionSlot = &LightRecord.pObjectLightIntersections;
				while ((*ppRenderableInsertionSlot) && RenderableUID > (*ppRenderableInsertionSlot)->pRenderableAttr->GetSceneHandle()->first)
					ppRenderableInsertionSlot = &(*ppRenderableInsertionSlot)->pNextRenderable;

				// The current light must not exist in the list of the renderable, because we are creating an intersection only now
				n_assert_dbg(!(*ppRenderableInsertionSlot) || RenderableUID < (*ppRenderableInsertionSlot)->pRenderableAttr->GetSceneHandle()->first);

				AttachObjectLightIntersection(pMatchingIntersection, ppLightInsertionSlot, ppRenderableInsertionSlot);
				ppLightInsertionSlot = &pMatchingIntersection->pNextLight;
				if (TrackingFlags & CRenderableAttribute::TrackLightContactChanges)
					IncrementVersion(RenderableRecord.ObjectLightIntersectionsVersion);
			}
			else
			{
				// Track relative movement of the contacting light if the renderable wants it (e.g. terrain does)
				if ((TrackingFlags & CRenderableAttribute::TrackLightRelativeMovement) &&
					(pMatchingIntersection->LightBoundsVersion != LightRecord.BoundsVersion ||
					 pMatchingIntersection->RenderableBoundsVersion != RenderableRecord.BoundsVersion))
				{
					IncrementVersion(RenderableRecord.ObjectLightIntersectionsVersion);
				}
			}

			pMatchingIntersection->LightBoundsVersion = LightRecord.BoundsVersion;
			pMatchingIntersection->RenderableBoundsVersion = RenderableRecord.BoundsVersion;
		}
		else if (!Intersects && pMatchingIntersection)
		{
			DetachObjectLightIntersection(pMatchingIntersection);
			_IntersectionPool.Destroy(pMatchingIntersection);
			if (TrackingFlags & CRenderableAttribute::TrackLightContactChanges)
				IncrementVersion(RenderableRecord.ObjectLightIntersectionsVersion);
		}
	}
}
//---------------------------------------------------------------------

// FIXME: major duplication! Could try to use indices Light = 0, Renderable = 1 => ppPrevNext[Renderable] etc and unify the code.
void CGraphicsScene::UpdateObjectLightIntersections(CLightAttribute& LightAttr)
{
	const auto LightUID = LightAttr.GetSceneHandle()->first;
	auto& LightRecord = LightAttr.GetSceneHandle()->second;

	// Only trackable lights with valid non-infinite bounds are expected here
	n_assert_dbg(LightRecord.TrackObjectLightIntersections && LightRecord.BoundsVersion);

	// Remember the last updated bounds, so that different views will not trigger redundant updates in a shared scene
	if (LightRecord.IntersectionBoundsVersion == LightRecord.BoundsVersion) return;
	LightRecord.IntersectionBoundsVersion = LightRecord.BoundsVersion;

	// TODO: instead of iterating through all _Renderables, can query them from octree! Will be much less instances to check!!!
	// Query by our attr's bounds and octree node. Go to all parents of the node and to only intersecting children, see UE:
	// - FindElementsWithBoundsTestInternal, GetIntersectingChildren
	//???how to handle not sorted collection, e.g. data from octree query?
	//???query to vector (stored in class to avoid per frame allocations) and sort pefore processing? Will O(query+sort) be better than O(bruteforce)?
	//???how UE avoids recreating already existing links? can't find. does it erase the proxy and then refill links when something changes?

	// An adapted version of DEM::Algo::SortedUnion for syncing with CObjectLightIntersection. Both collections are sorted by UID.
	// The logic is simplified in comparison with original SortedUnion because we guarantee that no intersections exist for removed objects and lights.
	CObjectLightIntersection** ppRenderableInsertionSlot = &LightRecord.pObjectLightIntersections;
	for (auto& [RenderableUID, RenderableRecord] : _Renderables)
	{
		CObjectLightIntersection* pMatchingIntersection;
		if (!(*ppRenderableInsertionSlot) || RenderableUID < (*ppRenderableInsertionSlot)->pRenderableAttr->GetSceneHandle()->first)
		{
			pMatchingIntersection = nullptr;
		}
		else // equal UIDs, matching intersection found
		{
			n_assert_dbg((*ppRenderableInsertionSlot) && (*ppRenderableInsertionSlot)->pRenderableAttr->GetSceneHandle()->first == RenderableUID);

			pMatchingIntersection = (*ppRenderableInsertionSlot);
			ppRenderableInsertionSlot = &pMatchingIntersection->pNextRenderable;

			// Skip if has up to date intersection (e.g. when both renderable and light get updated at the same frame)
			//???FIXME: need to check both renderable and light here? or updated renderable must be completely skipped?!
			if (pMatchingIntersection->LightBoundsVersion == LightRecord.BoundsVersion) continue;
		}

		// Don't erase existing connection in this case, tracking may be re-enabled later and connection may remain valid
		//!!!FIXME: if querying octree, there won't be objects with zero TrackObjectLightIntersections, so this condition won't be needed!
		if (!RenderableRecord.TrackObjectLightIntersections) continue;

		const auto TrackingFlags = static_cast<CRenderableAttribute*>(RenderableRecord.pAttr)->GetLightTrackingFlags();
		n_assert_dbg(TrackingFlags);

		const bool Intersects = static_cast<CLightAttribute*>(LightRecord.pAttr)->IntersectsWith(RenderableRecord.Sphere);
		if (Intersects)
		{
			if (!pMatchingIntersection)
			{
				pMatchingIntersection = _IntersectionPool.Construct();
				pMatchingIntersection->pRenderableAttr = static_cast<CRenderableAttribute*>(RenderableRecord.pAttr);
				pMatchingIntersection->pLightAttr = static_cast<CLightAttribute*>(LightRecord.pAttr);

				// Find the position in the light list of this renderable to preserve UID sorting
				// TODO PERF: now O(n). If critical, can switch from linked list to std::map for O(logN), but anyway clustered lighting should reduce workload a lot.
				// Could break sorting and insert in O(1) like in UE, but then need to clear and refill intersections each frame and resubmit to GPU too.
				auto ppLightInsertionSlot = &RenderableRecord.pObjectLightIntersections;
				while ((*ppLightInsertionSlot) && LightUID > (*ppLightInsertionSlot)->pLightAttr->GetSceneHandle()->first)
					ppLightInsertionSlot = &(*ppLightInsertionSlot)->pNextLight;

				// The current light must not exist in the list of the renderable, because we are creating an intersection only now
				n_assert_dbg(!(*ppLightInsertionSlot) || LightUID < (*ppLightInsertionSlot)->pLightAttr->GetSceneHandle()->first);

				AttachObjectLightIntersection(pMatchingIntersection, ppLightInsertionSlot, ppRenderableInsertionSlot);
				ppRenderableInsertionSlot = &pMatchingIntersection->pNextRenderable;
				if (TrackingFlags & CRenderableAttribute::TrackLightContactChanges)
					IncrementVersion(RenderableRecord.ObjectLightIntersectionsVersion);
			}
			else
			{
				// Track relative movement of the contacting light if the renderable wants it (e.g. terrain does)
				if ((TrackingFlags & CRenderableAttribute::TrackLightRelativeMovement) &&
					(pMatchingIntersection->LightBoundsVersion != LightRecord.BoundsVersion ||
					 pMatchingIntersection->RenderableBoundsVersion != RenderableRecord.BoundsVersion))
				{
					IncrementVersion(RenderableRecord.ObjectLightIntersectionsVersion);
				}
			}

			pMatchingIntersection->LightBoundsVersion = LightRecord.BoundsVersion;
			pMatchingIntersection->RenderableBoundsVersion = RenderableRecord.BoundsVersion;
		}
		else if (!Intersects && pMatchingIntersection)
		{
			DetachObjectLightIntersection(pMatchingIntersection);
			_IntersectionPool.Destroy(pMatchingIntersection);
			if (TrackingFlags & CRenderableAttribute::TrackLightContactChanges)
				IncrementVersion(RenderableRecord.ObjectLightIntersectionsVersion);
		}
	}
}
//---------------------------------------------------------------------

}
