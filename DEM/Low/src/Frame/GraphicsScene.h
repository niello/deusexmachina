#pragma once
#include <Data/SparseArray2.hpp>
#include <Math/AABB.h>
#include <System/Allocators/PoolAllocator.h>
#include <acl/math/math_types.h>
#include <map>

// Container for graphics objects - renderables and lights. Accelerated with a spatial partitioning tree.
// A loose octree is used currently.

namespace Math
{
	struct CSIMDFrustum;
}

namespace Scene
{
	class CNodeAttribute;
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

struct CObjectLightIntersection
{
	CLightAttribute*           pLightAttr;
	CRenderableAttribute*      pRenderableAttr;

	CObjectLightIntersection** ppPrevLightNext;
	CObjectLightIntersection*  pNextLight;
	CObjectLightIntersection** ppPrevRenderableNext;
	CObjectLightIntersection*  pNextRenderable;

	U32                        LightBoundsVersion;
	U32                        RenderableBoundsVersion;
};

class CGraphicsScene
{
public:

	struct CSpatialRecord
	{
		acl::Vector4_32           BoxCenter;
		acl::Vector4_32           BoxExtent;
		Scene::CNodeAttribute*    pAttr = nullptr;
		CObjectLightIntersection* pObjectLightIntersections = nullptr;
		TMorton                   NodeMortonCode = 0; // 0 is for objects outside the octree, 1 is for root, and longer codes are for child nodes
		float                     SphereRadius = 0.f; //!!!optimal center may be different! store acl::Vector4_32 with center and radius? then fix light intersection calls!
		U32                       NodeIndex = NO_SPATIAL_TREE_NODE;
		U32                       BoundsVersion = 1;  // 0 for invalid bounds (e.g. infinite)
		U32                       IntersectionBoundsVersion = 0;
		U16                       ObjectLightIntersectionsVersion = 0;
		U8                        TrackObjectLightIntersections = 0;
	};

	using HRecord = std::map<UPTR, CSpatialRecord>::iterator;

protected:

	Data::CSparseArray2<CSpatialTreeNode, U32>       _TreeNodes;
	std::unordered_map<TMorton, U32>                 _MortonToIndex;
	std::vector<decltype(_MortonToIndex)::node_type> _MortonToIndexPool;

	std::map<UPTR, CSpatialRecord>                   _Renderables;
	std::map<UPTR, CSpatialRecord>                   _Lights;
	std::vector<decltype(_Renderables)::node_type>   _ObjectNodePool;

	CPool<CObjectLightIntersection>                  _IntersectionPool;

	float _WorldExtent = 0.f; // Having all extents the same reduces calculation and makes moving object update frequency isotropic
	float _InvWorldSize = 0.f; // Cached 1 / (2 * _WorldExtent)
	float _SmallestExtent = 0.f; // Cached _WorldExtent / (1 << _MaxDepth), smallest extent that requires depth calculation on insertion
	U8    _MaxDepth = 0;

	UPTR  _NextRenderableUID = 1; // UID 0 means no UID assigned, so start from 1
	UPTR  _NextLightUID = 1;      // UID 0 means no UID assigned, so start from 1
	U32   _SpatialTreeRebuildVersion = 1; // Grows when existing nodes in _TreeNodes change, to invalidate visibility caches in views

	TMorton CalculateMortonCode(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent) const noexcept;
	U32     CreateNode(U32 FreeIndex, TMorton MortonCode, U32 ParentIndex);
	U32     AddSingleObjectToNode(TMorton NodeMortonCode, TMorton StopMortonCode);
	void    RemoveSingleObjectFromNode(U32 NodeIndex, TMorton NodeMortonCode, TMorton StopMortonCode);

	HRecord AddObject(std::map<UPTR, CSpatialRecord>& Storage, UPTR UID, const CAABB& GlobalBox, Scene::CNodeAttribute& Attr);
	void    UpdateObjectBounds(HRecord Handle, const CAABB& GlobalBox);
	void    RemoveObject(std::map<UPTR, CSpatialRecord>& Storage, HRecord Handle);

	void                      TrackObjectLightIntersections(CSpatialRecord& Record, bool Track);
	CObjectLightIntersection* CreateObjectLightIntersection(const CSpatialRecord& LightRecord, const CSpatialRecord& RenderableRecord);

public:

	void            Init(const vector3& Center, float Size, U8 HierarchyDepth);

	HRecord         AddRenderable(const CAABB& GlobalBox, CRenderableAttribute& RenderableAttr);
	void            UpdateRenderableBounds(HRecord Handle, const CAABB& GlobalBox) { UpdateObjectBounds(Handle, GlobalBox); }
	void            RemoveRenderable(HRecord Handle) { RemoveObject(_Renderables, Handle); }
	const auto&     GetRenderables() const { return _Renderables; }

	HRecord         AddLight(const CAABB& GlobalBox, CLightAttribute& LightAttr);
	void            UpdateLightBounds(HRecord Handle, const CAABB& GlobalBox) { UpdateObjectBounds(Handle, GlobalBox); }
	void            RemoveLight(HRecord Handle) { RemoveObject(_Lights, Handle); }
	const auto&     GetLights() const { return _Lights; }

	void            TestSpatialTreeVisibility(const Math::CSIMDFrustum& Frustum, std::vector<bool>& NodeVisibility) const;

	acl::Vector4_32 CalcNodeBounds(TMorton MortonCode) const;
	CAABB           GetNodeAABB(U32 NodeIndex, bool Loose = false) const;
	CAABB           GetNodeAABB(acl::Vector4_32Arg0 Bounds, bool Loose = false) const;
	U32             GetSpatialTreeRebuildVersion() const { return _SpatialTreeRebuildVersion; }

	void            TrackObjectLightIntersections(CRenderableAttribute& RenderableAttr, bool Track);
	void            TrackObjectLightIntersections(CLightAttribute& LightAttr, bool Track);
	void            UpdateObjectLightIntersections(CRenderableAttribute& RenderableAttr);
	void            UpdateObjectLightIntersections(CLightAttribute& LightAttr);
};

}
