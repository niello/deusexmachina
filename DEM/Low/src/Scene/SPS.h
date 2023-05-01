#pragma once
#include <Data/QuadTree.h>
#include <Data/Array.h>
#include <Data/SparseArray2.hpp>
#include <System/Allocators/PoolAllocator.h>
#include <acl/math/math_types.h>

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

struct CSpatialTreeNode
{
	acl::Vector4_32 Bounds; // Cx, Cy, Cz, Ecoeff for an octree
	U32             MortonCode;
	U32             ParentIndex;
	U32             SubtreeObjectCount;
};

constexpr auto NO_NODE = Data::CSparseArray2<CSpatialTreeNode, U32>::INVALID_INDEX;

struct CSPSRecord
{
	CSPSRecord*		pNext = nullptr;
	CSPSRecord*		pPrev = nullptr;
	CSPSNode*		pSPSNode = nullptr;
	CNodeAttribute*	pUserData = nullptr;
	CAABB			GlobalBox;

	///////// NEW RENDER /////////
	//float CenterX;
	//float CenterZ;
	//float HalfExtentX;
	//float HalfExtentZ;
	U32 NodeMortonCode = 0; // 0 is for objects outside the octree, 1 is for root, then 100, 101, 110, 111 etc
	U32 NodeIndex = NO_NODE;
};

class CSPS //???rename to CGfxLevel? CRenderLevel etc? Not exactly SPS, but a thing one step above it.
{
protected:

	CPool<CSPSRecord, 512> RecordPool;

	void		QueryObjectsInsideFrustum(CSPSNode* pNode, const matrix44& ViewProj, CArray<CNodeAttribute*>& OutObjects, EClipStatus Clip) const;

	///////// NEW RENDER /////////
	Data::CSparseArray2<CSpatialTreeNode, U32> _TreeNodes;
	std::unordered_map<U32, U32> _MortonToIndex; //???TODO PERF: map or unordered_map?
	std::vector<std::unordered_map<U32, U32>::node_type> _MappingPool;

	//!!!TODO: separate by type - renderables, lights, ambient etc!
	//SPSRecord is not a renderable cache node! They are per view, and here we have per-scene thing!
	//Render cache can be pre-sorted e.g. by material, and re-sort parts FtB/BtF based on camera when needed, making additional index lists.
	//???need to sort records here?
	//Visibility flag inside render cache node or in a separate array in a CView? But still not here!
	//???!!!store object's AABB in center/half-extents form even outside here?! would save calculations, and in a frustum test too!
	std::vector<CSPSRecord> _Records;
	vector3 _WorldCenter; // TODO: requires allocation alignment! acl::Vector4_32 _WorldBounds; // Cx, Cy, Cz, Ecoeff = 1.f
	float _WorldExtent = 0.f; // Having all extents the same reduces calculation and makes moving object update frequency isotropic
	U8 _MaxDepth = 0;

	U32 CalculateQuadtreeMortonCode(float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ) const noexcept;
	U32 CreateNode(U32 FreeIndex, U32 MortonCode, U32 ParentIndex);
	U32 AddSingleObject(U32 NodeMortonCode, U32 StopMortonCode);
	void RemoveSingleObject(U32 NodeIndex, U32 NodeMortonCode, U32 StopMortonCode);
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

	void		QueryObjectsInsideFrustum(const matrix44& ViewProj, CArray<CNodeAttribute*>& OutObjects) const;
	//!!!add inside sphere for omni (point) lights!

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
