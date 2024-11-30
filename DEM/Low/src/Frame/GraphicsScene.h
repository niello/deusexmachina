#pragma once
#include <Data/SparseArray2.hpp>
#include <Math/SIMDMath.h>
#include <System/Allocators/PoolAllocator.h>
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
#if DEM_CPU_64
using TSceneMorton = U64;
using TSceneCellDim = U32; // Max 31 bits for quadtree, 21 bits for octree
#else
using TSceneMorton = U32;
using TSceneCellDim = U16; // Max 15 bits for quadtree, 10 bits for octree
#endif
static constexpr size_t TREE_DIMENSIONS = 3;
static constexpr U8 TREE_MAX_DEPTH = (sizeof(TSceneMorton) * 8 - 1) / TREE_DIMENSIONS;

struct CSpatialTreeNode
{
	rtm::vector4f Bounds; // Cx, Cy, Cz, Ecoeff for an octree
	TSceneMorton    MortonCode;
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
		Math::CAABB               Box;
		rtm::vector4f             Sphere;
		Scene::CNodeAttribute*    pAttr = nullptr;
		CObjectLightIntersection* pObjectLightIntersections = nullptr;
		TSceneMorton              NodeMortonCode = 0;                  // 0 is for objects outside the octree, 1 is for root, and longer codes are for child nodes
		U32                       NodeIndex = NO_SPATIAL_TREE_NODE;
		U32                       BoundsVersion = 1;                   // 0 for invalid bounds (e.g. infinite)
		U32                       IntersectionBoundsVersion = 0;
		U16                       ObjectLightIntersectionsVersion = 0; // Source, syncs with IRenderable::ObjectLightIntersectionsVersion
		U8                        TrackObjectLightIntersections = 0;   // A counter for spawned IRenderables with light tracking enabled
	};

	using HRecord = std::map<UPTR, CSpatialRecord>::iterator;

protected:

	Data::CSparseArray2<CSpatialTreeNode, U32>       _TreeNodes;
	std::unordered_map<TSceneMorton, U32>                 _MortonToIndex;
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

	TSceneMorton    CalculateMortonCode(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const noexcept;
	U32             CreateNode(U32 FreeIndex, TSceneMorton MortonCode, U32 ParentIndex);
	U32             AddSingleObjectToNode(TSceneMorton NodeMortonCode, TSceneMorton StopMortonCode);
	void            RemoveSingleObjectFromNode(U32 NodeIndex, TSceneMorton NodeMortonCode, TSceneMorton StopMortonCode);

	HRecord         AddObject(std::map<UPTR, CSpatialRecord>& Storage, UPTR UID, const Math::CAABB& GlobalBox, rtm::vector4f_arg0 GlobalSphere, Scene::CNodeAttribute& Attr);
	void            UpdateObjectBounds(HRecord Handle, const Math::CAABB& GlobalBox, rtm::vector4f_arg0 GlobalSphere);
	void            RemoveObject(std::map<UPTR, CSpatialRecord>& Storage, HRecord Handle);

	void            TrackObjectLightIntersections(CSpatialRecord& Record, bool Track);

public:

	void            Init(const rtm::vector4f& Center, float Size, U8 HierarchyDepth);

	HRecord         AddRenderable(const Math::CAABB& GlobalBox, CRenderableAttribute& RenderableAttr);
	void            UpdateRenderableBounds(HRecord Handle, const Math::CAABB& GlobalBox);
	void            RemoveRenderable(HRecord Handle);
	const auto&     GetRenderables() const { return _Renderables; }

	HRecord         AddLight(const Math::CAABB& GlobalBox, rtm::vector4f_arg0 GlobalSphere, CLightAttribute& LightAttr);
	void            UpdateLightBounds(HRecord Handle, const Math::CAABB& GlobalBox, rtm::vector4f_arg0 GlobalSphere);
	void            RemoveLight(HRecord Handle);
	const auto&     GetLights() const { return _Lights; }

	void            TestSpatialTreeVisibility(const Math::CSIMDFrustum& Frustum, std::vector<bool>& NodeVisibility) const;

	rtm::vector4f   CalcNodeBounds(TSceneMorton MortonCode) const;
	Math::CAABB     GetNodeAABB(U32 NodeIndex, bool Loose = false) const;
	Math::CAABB     GetNodeAABB(rtm::vector4f_arg0 Bounds, bool Loose = false) const;
	U32             GetSpatialTreeRebuildVersion() const { return _SpatialTreeRebuildVersion; }

	void            TrackObjectLightIntersections(CRenderableAttribute& RenderableAttr, bool Track);
	void            TrackObjectLightIntersections(CLightAttribute& LightAttr, bool Track);
	void            UpdateObjectLightIntersections(CRenderableAttribute& RenderableAttr);
	void            UpdateObjectLightIntersections(CLightAttribute& LightAttr);
};

}
