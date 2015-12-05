#pragma once
#ifndef __DEM_L1_SCENE_SPS_H__
#define __DEM_L1_SCENE_SPS_H__

#include <Data/QuadTree.h>
#include <Data/Array.h>

// Spatial partitioning structure for accelerated spatial queries on a scene or any other object set
// CSPS       - spatial partitioning structure, that stores objects spatially arranged
// CSPSNode   - SPS hierarchy building block, internal
// CSPSCell   - user data storage incapsulated in a node
// CSPSRecord - user data with its spatial properties and some additional fields

//???store CNodeAttribute* / CRTTIBaseClass* instead of void*? or how to safely convert pointers?
//???or templated SPS?

namespace Scene
{
struct CSPSRecord;

struct CSPSCell
{
	typedef CArray<CSPSRecord*>::CIterator CIterator;

	//???use persistent array? like a handle manager or array with NULL-filling instead of shifting?
	CArray<CSPSRecord*> Objects;

	// NB: no persistent handle for arrays
	CIterator	Add(CSPSRecord* const & Object) { Objects.Add(Object); return NULL; }
	bool		RemoveByValue(CSPSRecord* const & Object) { return Objects.RemoveByValue(Object); }
	void		Remove(CIterator It) { Sys::Error("CSPSCell::Remove() > There are no persistent handles for arrays due to possible data move!"); }
	CIterator	Find(CSPSRecord* const & Object) const { return NULL; } // Never used since handles aren't used
};

typedef Data::CQuadTree<CSPSRecord*, CSPSCell> CSPSQuadTree;
typedef CSPSQuadTree::CNode CSPSNode;

struct CSPSRecord
{
	CSPSNode*	pSPSNode;
	void*		pUserData;
	CAABB		GlobalBox; //???or recalculate into a center-halfsize form on SetAABB(const CAABB& GlobalBox)?!

	CSPSRecord(): pUserData(NULL), pSPSNode(NULL) {} 
	CSPSRecord(const CSPSRecord& Rec): pUserData(Rec.pUserData), GlobalBox(Rec.GlobalBox), pSPSNode(Rec.pSPSNode) {}
	~CSPSRecord() { if (pSPSNode) pSPSNode->RemoveByValue(this); }

	void GetDimensions(float& CenterX, float& CenterZ, float& HalfSizeX, float& HalfSizeZ) const;
};

class CSPS
{
protected:

	//!!!record pool! or use small object allocator!

	void QueryObjectsInsideFrustum(CSPSNode* pNode, const matrix44& ViewProj, CArray<void*>& OutObjects, EClipStatus Clip) const;

public:

	CArray<void*>	OversizedObjects;
	CSPSQuadTree	QuadTree;
	float			SceneMinY;
	float			SceneMaxY;

	//!!!CSPSRecord* CreateRecord(AABB, userdata) const; (pool)!
	void AddObjectRecord(CSPSRecord* pRecord);
	void UpdateObjectRecord(CSPSRecord* pRecord);

	//???return SPS records? if so, oversized objects need records too!
	void QueryObjectsInsideFrustum(const matrix44& ViewProj, CArray<void*>& OutObjects) const;
};

inline void CSPSRecord::GetDimensions(float& CenterX, float& CenterZ, float& HalfSizeX, float& HalfSizeZ) const
{
	float HalfMinX = GlobalBox.Min.x * 0.5f;
	float HalfMinZ = GlobalBox.Min.z * 0.5f;
	float HalfMaxX = GlobalBox.Max.x * 0.5f;
	float HalfMaxZ = GlobalBox.Max.z * 0.5f;
	CenterX = HalfMaxX + HalfMinX;
	CenterZ = HalfMaxZ + HalfMinZ;
	HalfSizeX = HalfMaxX - HalfMinX;
	HalfSizeZ = HalfMaxZ - HalfMinZ;
}
//---------------------------------------------------------------------

inline void CSPS::AddObjectRecord(CSPSRecord* pRecord)
{
	float CenterX, CenterZ, HalfSizeX, HalfSizeZ;
	pRecord->GetDimensions(CenterX, CenterZ, HalfSizeX, HalfSizeZ);
	QuadTree.AddObject(pRecord, CenterX, CenterZ, HalfSizeX, HalfSizeZ, pRecord->pSPSNode);
}
//---------------------------------------------------------------------

inline void CSPS::UpdateObjectRecord(CSPSRecord* pRecord)
{
	float CenterX, CenterZ, HalfSizeX, HalfSizeZ;
	pRecord->GetDimensions(CenterX, CenterZ, HalfSizeX, HalfSizeZ);
	QuadTree.UpdateObject(pRecord, CenterX, CenterZ, HalfSizeX, HalfSizeZ, pRecord->pSPSNode);
}
//---------------------------------------------------------------------

}

#endif
