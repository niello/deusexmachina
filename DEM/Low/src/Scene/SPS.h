#pragma once
#include <Data/QuadTree.h>
#include <Data/Array.h>
#include <Data/SparseArray2.hpp>
#include <System/Allocators/PoolAllocator.h>
#include <acl/math/math_types.h>
#include <map>

// Spatial partitioning structure for accelerated spatial queries on a scene
// CSPS       - spatial partitioning structure, that stores objects spatially arranged
// CSPSNode   - SPS hierarchy building block, internal
// CSPSCell   - user data storage incapsulated in a node
// CSPSRecord - user data with its spatial properties and some additional fields

namespace Scene
{
class CNodeAttribute;
struct CSPSRecord;

// Implements a doubly-linked list with an external pool allocator.
// We don't reuse CList<T> here because it requires a modification to store an allocator object,
// which adds another template argument and wastes both space and time in a case of standard stateless
// allocation from the main heap.
class CSPSCell
{
protected:

	void Remove(CSPSRecord* pRec);

public:

	class CIterator
	{
	public:

		CSPSRecord* pNode;

		CIterator(): pNode(nullptr) {}
		CIterator(CSPSRecord* Node): pNode(Node) {}
		CIterator(const CIterator& Other): pNode(Other.pNode) {}

		bool				operator ==(const CIterator& Other) const { return pNode == Other.pNode; }
		bool				operator !=(const CIterator& Other) const { return pNode != Other.pNode; }
		const CIterator&	operator =(const CIterator& Other) { pNode = Other.pNode; return *this; }
		const CIterator&	operator ++();
		CIterator			operator ++(int);
		const CIterator&	operator --();
		CIterator			operator --(int);
							operator bool() const { return !!pNode; }
		CSPSRecord*			operator ->() const { return pNode; }
		CSPSRecord*			operator *() const { return pNode; }
	};

	CSPSRecord* pFront;

	CSPSCell(): pFront(nullptr) {}
	~CSPSCell() { n_assert(!pFront); } // All nodes must be removed and deallocated by SPS

	CIterator	Add(CSPSRecord* Object);
	bool		RemoveByValue(CSPSRecord* Object);
	void		Remove(CIterator It) { Remove(It.pNode); }
	CIterator	Find(CSPSRecord* Object) const;
	bool		IsEmpty() const { return !pFront; }
	CIterator	Begin() const { return CIterator(pFront); }
	CIterator	End() const { return CIterator(nullptr); }
};

typedef Data::CQuadTree<CSPSRecord*, CSPSCell> CSPSQuadTree;
typedef CSPSQuadTree::CNode CSPSNode;

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

struct CSPSRecord
{
	///////// NEW RENDER /////////
	acl::Vector4_32 BoxCenter;
	acl::Vector4_32 BoxExtent;
	TMorton         NodeMortonCode = 0; // 0 is for objects outside the octree, 1 is for root, and longer codes for child nodes
	U32             NodeIndex = NO_SPATIAL_TREE_NODE;
	U32             BoundsVersion = 1;
	//////////////////////////////

	CSPSRecord*		pNext = nullptr;
	CSPSRecord*		pPrev = nullptr;
	CSPSNode*		pSPSNode = nullptr;
	CNodeAttribute*	pUserData = nullptr;
	CAABB			GlobalBox;
};

class CSPS //???rename to CGfxLevel? CRenderLevel etc? Not exactly SPS, but a thing one step above it.
{
protected:

	CPool<CSPSRecord, 512> RecordPool;

	void		QueryObjectsInsideFrustum(CSPSNode* pNode, const matrix44& ViewProj, CArray<CNodeAttribute*>& OutObjects, EClipStatus Clip) const;

	///////// NEW RENDER /////////
	Data::CSparseArray2<CSpatialTreeNode, U32> _TreeNodes;
	std::unordered_map<TMorton, U32> _MortonToIndex;
	std::vector<std::unordered_map<TMorton, U32>::node_type> _MortonToIndexPool;

	//!!!TODO: separate by type - renderables, lights, ambient etc!
	std::map<UPTR, CSPSRecord*> _Objects; // TODO: could try to use std::set
	vector3 _WorldCenter;
	float _WorldExtent = 0.f; // Having all extents the same reduces calculation and makes moving object update frequency isotropic
	float _InvWorldSize = 0.f; // Cached 1 / (2 * _WorldExtent)
	float _SmallestExtent = 0.f; // Cached _WorldExtent / (1 << _MaxDepth), smallest extent that requires depth calculation on insertion
	U8 _MaxDepth = 0;

	UPTR _NextUID = 1; // UID 0 means no UID assigned, so start from 1

	TMorton CalculateMortonCode(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent) const noexcept;
	U32     CreateNode(U32 FreeIndex, TMorton MortonCode, U32 ParentIndex);
	U32     AddSingleObject(TMorton NodeMortonCode, TMorton StopMortonCode);
	void    RemoveSingleObject(U32 NodeIndex, TMorton NodeMortonCode, TMorton StopMortonCode);
	//////////////////////////////

public:

	CArray<CNodeAttribute*>	OversizedObjects;
	CSPSQuadTree			QuadTree;
	float					SceneMinY = 0.f;
	float					SceneMaxY = 0.f;

	void		Init(const vector3& Center, float Size, U8 HierarchyDepth);
	CSPSRecord*	AddRecord(const CAABB& GlobalBox, CNodeAttribute* pUserData);
	void		UpdateRecord(CSPSRecord* pRecord);
	void		RemoveRecord(CSPSRecord* pRecord);

	const auto&     GetObjects() const { return _Objects; }
	acl::Vector4_32 CalcNodeBounds(TMorton MortonCode) const;
	CAABB           GetNodeAABB(U32 NodeIndex, bool Loose = false) const;
	CAABB           GetNodeAABB(acl::Vector4_32Arg0 Bounds, bool Loose = false) const;

	void		QueryObjectsInsideFrustum(const matrix44& ViewProj, CArray<CNodeAttribute*>& OutObjects) const;
	//!!!add InsideSphere for querying objects touching omni (point) lights! take into account only visible tree nodes!

	///////// NEW RENDER /////////
	//!!!TODO: use prepared frustum planes instead of matrix!
	void TestSpatialTreeVisibility(const matrix44& ViewProj, std::vector<bool>& NodeVisibility) const;
	//////////////////////////////
};

inline void GetDimensions(const CAABB& Box, float& CenterX, float& CenterZ, float& HalfSizeX, float& HalfSizeZ)
{
	float HalfMinX = Box.Min.x * 0.5f;
	float HalfMinZ = Box.Min.z * 0.5f;
	float HalfMaxX = Box.Max.x * 0.5f;
	float HalfMaxZ = Box.Max.z * 0.5f;
	CenterX = HalfMaxX + HalfMinX;
	CenterZ = HalfMaxZ + HalfMinZ;
	HalfSizeX = HalfMaxX - HalfMinX;
	HalfSizeZ = HalfMaxZ - HalfMinZ;
}
//---------------------------------------------------------------------

inline const CSPSCell::CIterator& CSPSCell::CIterator::operator ++()
{
	n_assert_dbg(pNode);
	pNode = pNode->pNext;
	return *this;
}
//---------------------------------------------------------------------

inline CSPSCell::CIterator CSPSCell::CIterator::operator ++(int)
{
	n_assert_dbg(pNode);
	CIterator Tmp(pNode);
	pNode = pNode->pNext;
	return Tmp;
}
//---------------------------------------------------------------------

inline const CSPSCell::CIterator& CSPSCell::CIterator::operator --()
{
	n_assert_dbg(pNode);
	pNode = pNode->pPrev;
	return *this;
}
//---------------------------------------------------------------------

inline CSPSCell::CIterator CSPSCell::CIterator::operator --(int)
{
	n_assert_dbg(pNode);
	CIterator Tmp(pNode);
	pNode = pNode->pPrev;
	return Tmp;
}
//---------------------------------------------------------------------

}

namespace Data
{

// We can't use the default implementation, where we add first, than remove, because we use
// shared nodes and don't want to reallocate them at each move operation.
template<>
inline Scene::CSPSCell::CIterator Scene::CSPSQuadTree::MoveElement(Scene::CSPSCell& From, Scene::CSPSCell& To, Scene::CSPSCell::CIterator Handle)
{
	From.Remove(Handle);
	return To.Add(Handle.pNode);
}
//---------------------------------------------------------------------

}
