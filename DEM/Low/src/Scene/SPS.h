#pragma once
#ifndef __DEM_L1_SCENE_SPS_H__
#define __DEM_L1_SCENE_SPS_H__

#include <Data/QuadTree.h>
#include <Data/Array.h>
#include <System/Allocators/PoolAllocator.h>

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

struct CSPSRecord
{
	CSPSRecord*		pNext = nullptr;
	CSPSRecord*		pPrev = nullptr;
	CSPSNode*		pSPSNode = nullptr;
	CNodeAttribute*	pUserData = nullptr;
	CAABB			GlobalBox;
};

class CSPS
{
protected:

	CPool<CSPSRecord, 512> RecordPool;

	void		QueryObjectsInsideFrustum(CSPSNode* pNode, const matrix44& ViewProj, CArray<CNodeAttribute*>& OutObjects, EClipStatus Clip) const;

public:

	CArray<CNodeAttribute*>	OversizedObjects;
	CSPSQuadTree			QuadTree;
	float					SceneMinY = 0.f;
	float					SceneMaxY = 0.f;

	void		Init(const vector3& Center, const vector3& Size, UPTR HierarchyDepth);
	CSPSRecord*	AddRecord(const CAABB& GlobalBox, CNodeAttribute* pUserData);
	void		UpdateRecord(CSPSRecord* pRecord);
	void		RemoveRecord(CSPSRecord* pRecord);

	void		QueryObjectsInsideFrustum(const matrix44& ViewProj, CArray<CNodeAttribute*>& OutObjects) const;
	//!!!add inside sphere for omni (point) lights!
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

inline void CSPS::Init(const vector3& Center, const vector3& Size, UPTR HierarchyDepth)
{
	SceneMinY = Center.y - Size.y * 0.5f;
	SceneMaxY = Center.y + Size.y * 0.5f;
	QuadTree.Build(Center.x, Center.z, Size.x, Size.z, HierarchyDepth);
}
//---------------------------------------------------------------------

inline void CSPS::UpdateRecord(CSPSRecord* pRecord)
{
	float CenterX, CenterZ, HalfSizeX, HalfSizeZ;
	GetDimensions(pRecord->GlobalBox, CenterX, CenterZ, HalfSizeX, HalfSizeZ);
	QuadTree.UpdateHandle(CSPSCell::CIterator(pRecord), CenterX, CenterZ, HalfSizeX, HalfSizeZ, pRecord->pSPSNode);
}
//---------------------------------------------------------------------

inline void CSPS::RemoveRecord(CSPSRecord* pRecord)
{
	if (pRecord->pSPSNode) pRecord->pSPSNode->RemoveByHandle(CSPSCell::CIterator(pRecord));
	RecordPool.Destroy(pRecord);
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

#endif
