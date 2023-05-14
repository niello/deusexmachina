#pragma once
#include <Data/SparseArray2.hpp>
#include <Math/AABB.h>
#include <acl/math/math_types.h>
#include <map>

// Container for graphics objects - renderables and lights. Accelerated with a spatial partitioning tree.
// A loose octree is used currently.

namespace Math
{
	struct CSIMDFrustum;
}

namespace Frame
{
class CRenderableAttribute;
class CLightAttribute;

// TODO: make template class TTree<DIMENSIONS, CODE_TYPE> and move these constants and quadtree/octree utility methods into it?
#if DEM_64
using TMorton = U64;
using TCellDim = U32; // Max 31 bits for quadtree, 21 bits for octree
#else
using TMorton = U32;
using TCellDim = U16; // Max 15 bits for quadtree, 10 bits for octree
#endif
static inline constexpr size_t TREE_DIMENSIONS = 3;
static inline constexpr U8 TREE_MAX_DEPTH = (sizeof(TMorton) * 8 - 1) / TREE_DIMENSIONS;

struct CSpatialTreeNode
{
	acl::Vector4_32 Bounds; // Cx, Cy, Cz, Ecoeff for an octree
	TMorton         MortonCode;
	U32             ParentIndex;
	U32             SubtreeObjectCount;
};

constexpr auto NO_SPATIAL_TREE_NODE = Data::CSparseArray2<CSpatialTreeNode, U32>::INVALID_INDEX;

class CGraphicsScene
{
public:

	struct CRenderableRecord
	{
		acl::Vector4_32 BoxCenter;
		acl::Vector4_32 BoxExtent;
		CRenderableAttribute* pRenderableAttr = nullptr;
		TMorton         NodeMortonCode = 0; // 0 is for objects outside the octree, 1 is for root, and longer codes are for child nodes
		U32             NodeIndex = NO_SPATIAL_TREE_NODE;
		U32             BoundsVersion = 1;
		bool            BoundsValid = false;
	};

	struct CLightRecord
	{
	};

protected:

	Data::CSparseArray2<CSpatialTreeNode, U32> _TreeNodes;
	std::unordered_map<TMorton, U32> _MortonToIndex;
	std::vector<decltype(_MortonToIndex)::node_type> _MortonToIndexPool;

	std::map<UPTR, CRenderableRecord> _Renderables; // TODO: if cleared, need to clear iterators in attributes first!
	std::vector<decltype(_Renderables)::node_type> _RenderableNodePool;

	std::map<UPTR, CLightRecord> _Lights; // TODO: if cleared, need to clear iterators in attributes first!
	std::vector<decltype(_Lights)::node_type> _LightNodePool;

	vector3 _WorldCenter;
	float _WorldExtent = 0.f; // Having all extents the same reduces calculation and makes moving object update frequency isotropic
	float _InvWorldSize = 0.f; // Cached 1 / (2 * _WorldExtent)
	float _SmallestExtent = 0.f; // Cached _WorldExtent / (1 << _MaxDepth), smallest extent that requires depth calculation on insertion
	U8 _MaxDepth = 0;

	UPTR _NextUID = 1; // UID 0 means no UID assigned, so start from 1

	TMorton CalculateMortonCode(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent) const noexcept;
	U32     CreateNode(U32 FreeIndex, TMorton MortonCode, U32 ParentIndex);
	U32     AddSingleObjectToNode(TMorton NodeMortonCode, TMorton StopMortonCode);
	void    RemoveSingleObjectFromNode(U32 NodeIndex, TMorton NodeMortonCode, TMorton StopMortonCode);

public:

	using HRenderable = decltype(_Renderables)::iterator;
	using HLight = decltype(_Lights)::iterator;

	void            Init(const vector3& Center, float Size, U8 HierarchyDepth);

	HRenderable     AddRenderable(const CAABB& GlobalBox, CRenderableAttribute* pRenderableAttr);
	void            UpdateRenderable(HRenderable Handle, const CAABB& GlobalBox);
	void            RemoveRenderable(HRenderable Handle);
	auto            GetInvalidRenderableRecordHandle() const { return _Renderables.cend(); }

	HLight          AddLight(const CAABB& GlobalBox, CLightAttribute* pLightAttr);
	void            UpdateLight(HLight Handle, const CAABB& GlobalBox);
	void            RemoveLight(HLight Handle);
	auto            GetInvalidLightRecordHandle() const { return _Lights.cend(); }

	const auto&     GetRenderables() const { return _Renderables; }
	acl::Vector4_32 CalcNodeBounds(TMorton MortonCode) const;
	CAABB           GetNodeAABB(U32 NodeIndex, bool Loose = false) const;
	CAABB           GetNodeAABB(acl::Vector4_32Arg0 Bounds, bool Loose = false) const;

	void            TestSpatialTreeVisibility(const Math::CSIMDFrustum& Frustum, std::vector<bool>& NodeVisibility) const;
};

}
