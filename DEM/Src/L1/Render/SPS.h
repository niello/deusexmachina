#pragma once
#ifndef __DEM_L1_SCENE_SPS_H__
#define __DEM_L1_SCENE_SPS_H__

#include <Render/RenderObject.h>
#include <Render/Light.h>
#include <Data/QuadTree.h>

// Spatial partitioning structure specialization for culling
// CSPS       - spatial partitioning structure, that stores render objects spatially arranged
// CSPSNode   - one node in an SPS hierarchy, internal
// CSPSCell   - storage used by a node to store user data
// CSPSRecord - render object (with some additional data) as it is stored in the spatial partitioning structure

namespace Render
{
struct CSPSRecord;

struct CSPSCell
{
	typedef CSPSRecord** CIterator;

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
	const Scene::CNodeAttribute&	Attr;
	CAABB							GlobalBox;
	CSPSNode*						pSPSNode;

	CSPSRecord(const Scene::CNodeAttribute& NodeAttr): Attr(NodeAttr), pSPSNode(NULL) {} 
	CSPSRecord(const CSPSRecord& Rec): Attr(Rec.Attr), GlobalBox(Rec.GlobalBox), pSPSNode(Rec.pSPSNode) {}
	~CSPSRecord() { if (pSPSNode) pSPSNode->RemoveByValue(this); }

	//???use node attr flags to improve speed? anyway both render objects and lights have additional flags,
	//so we can use one flag to make difference before render objects lights despite of their subclassing.
	bool IsRenderObject() const { return Attr.IsA(CRenderObject::RTTI); }
	bool IsLight() const { return Attr.IsA(CLight::RTTI); }
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
	n_assert(Attr.GetNode());
	const vector3& Pos = Attr.GetNode()->GetWorldPosition();
	CenterX = Pos.x;
	CenterZ = Pos.z;
	HalfSizeX = (GlobalBox.Max.x - GlobalBox.Min.x) * 0.5f;
	HalfSizeZ = (GlobalBox.Max.z - GlobalBox.Min.z) * 0.5f;
}
//---------------------------------------------------------------------

// NB: no persistent handle for arrays
inline CSPSCell::CIterator CSPSCell::Add(CSPSRecord* const & Object)
{
	if (Object->IsRenderObject())
	{
		Objects.Add(Object);
		return NULL;
	}
	if (Object->IsLight())
	{
		Lights.Add(Object);
		return NULL;
	}
	Sys::Error("CSPSCell::Add() > Object passed is not a render object nor a light source\n");
	return NULL;
}
//---------------------------------------------------------------------

// Remove by value
inline bool CSPSCell::RemoveByValue(CSPSRecord* const & Object)
{
	if (Object->IsRenderObject()) return Objects.RemoveByValue(Object);
	if (Object->IsLight()) return Lights.RemoveByValue(Object);
	FAIL;
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
