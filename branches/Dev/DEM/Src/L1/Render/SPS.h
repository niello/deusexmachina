#pragma once
#ifndef __DEM_L1_SCENE_SPS_H__
#define __DEM_L1_SCENE_SPS_H__

#include <Data/QuadTree.h>
#include <Data/Array.h>

// Spatial partitioning structure specialization for culling
// CSPS       - spatial partitioning structure, that stores render objects spatially arranged
// CSPSNode   - one node in an SPS hierarchy, internal
// CSPSCell   - storage used by a node to store user data
// CSPSRecord - render object (with some additional data) as it is stored in the spatial partitioning structure

namespace Render
{
class CRenderObject;
class CLight;
struct CSPSRecord;

struct CSPSCell
{
	typedef CArray<CSPSRecord*>::CIterator CIterator;

	CArray<CSPSRecord*> Objects;
	CArray<CSPSRecord*> Lights;

	CIterator	Add(CSPSRecord* const & Object);
	bool		RemoveByValue(CSPSRecord* const & Object);
	void		Remove(CIterator It) { Sys::Error("CSPSCell::Remove() > There are no persistent handles for arrays due to possible data move!"); }
	CIterator	Find(CSPSRecord* const & Object) const { return NULL; } // Never used since handles aren't used
};

typedef Data::CQuadTree<CSPSRecord*, CSPSCell> CSPSQuadTree;
typedef CSPSQuadTree::CNode CSPSNode;

struct CSPSRecord
{
	CSPSNode*	pSPSNode;
	void*		pUserData;
	CAABB		GlobalBox;
	bool		IsLight;	// Allows us to store lights and geometry in two different arrays for faster queries (//???is really such a benefit?)

	CSPSRecord(): pUserData(NULL), pSPSNode(NULL) {} 
	CSPSRecord(const CSPSRecord& Rec): pUserData(Rec.pUserData), GlobalBox(Rec.GlobalBox), pSPSNode(Rec.pSPSNode) {}
	~CSPSRecord() { if (pSPSNode) pSPSNode->RemoveByValue(this); }

	void GetDimensions(float& CenterX, float& CenterZ, float& HalfSizeX, float& HalfSizeZ) const;
};

class CSPS
{
protected:

	//!!!record pool! or use small object allocator!

	void QueryVisibleObjectsAndLights(CSPSNode* pNode, const matrix44& ViewProj, CArray<CRenderObject*>* OutObjects, CArray<CLight*>* OutLights = NULL, EClipStatus Clip = Clipped) const;

public:

	CArray<CRenderObject*>	AlwaysVisibleObjects;
	CArray<CLight*>			AlwaysVisibleLights;
	CSPSQuadTree			QuadTree;
	float					SceneMinY;
	float					SceneMaxY;

	//!!!CSPSRecord* CreateRecord() const; (pool)!
	void AddObjectRecord(CSPSRecord* pRecord);
	void UpdateObjectRecord(CSPSRecord* pRecord);

	void QueryVisibleObjectsAndLights(const matrix44& ViewProj, CArray<CRenderObject*>* OutObjects, CArray<CLight*>* OutLights = NULL) const;
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

// NB: no persistent handle for arrays
inline CSPSCell::CIterator CSPSCell::Add(CSPSRecord* const & Object)
{
	if (Object->IsLight) Lights.Add(Object);
	else Objects.Add(Object);
	return NULL;
}
//---------------------------------------------------------------------

inline bool CSPSCell::RemoveByValue(CSPSRecord* const & Object)
{
	if (Object->IsLight) return Lights.RemoveByValue(Object);
	else return Objects.RemoveByValue(Object);
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
