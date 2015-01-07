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
	typedef CSPSRecord* CIterator;

	CArray<CSPSRecord*> Objects;
	CArray<CSPSRecord*> Lights;

	CIterator	Add(CSPSRecord* const & Object);
	bool		RemoveByValue(CSPSRecord* const & Object);
	void		Remove(CIterator It) { Sys::Error("There are no persistent handles for arrays due to possible data move!"); }
	CIterator	Find(CSPSRecord* const & Object) const { return NULL; } //???!!!implement?
};

class CSPS: public Data::CQuadTree<CSPSRecord*, CSPSCell>
{
protected:

	CNode AlwaysVisible; // For always visible objects (skybox, may be terrain etc) and lights (directional)

public:

	CHandle	AddAlwaysVisibleObject(CSPSRecord* Object) { return AlwaysVisible.AddObject(Object); }
	void	RemoveAlwaysVisibleByValue(CSPSRecord* Object);
	void	RemoveAlwaysVisibleByHandle(CHandle Handle);
};

typedef CSPS::CNode CSPSNode;

struct CSPSRecord
{
	Scene::CNodeAttribute&	Attr;
	CAABB					GlobalBox;
	CSPSNode*				pSPSNode;

	CSPSRecord(Scene::CNodeAttribute& NodeAttr): Attr(NodeAttr), pSPSNode(NULL) {} 
	CSPSRecord(const CSPSRecord& Rec): Attr(Rec.Attr), GlobalBox(Rec.GlobalBox), pSPSNode(Rec.pSPSNode) {} 

	bool		IsRenderObject() const { return Attr.IsA(CRenderObject::RTTI); }
	bool		IsLight() const { return Attr.IsA(CLight::RTTI); }

	void		GetCenter(vector2& Out) const;
	void		GetHalfSize(vector2& Out) const;
	CSPSNode*	GetQuadTreeNode() const { return pSPSNode; }
	void		SetQuadTreeNode(CSPSNode* pNode) { pSPSNode = pNode; }
};

inline void CSPSRecord::GetCenter(vector2& Out) const
{
	n_assert(Attr.GetNode());
	const vector3& Pos = Attr.GetNode()->GetWorldPosition();
	Out.x = Pos.x;
	Out.y = Pos.z;
}
//---------------------------------------------------------------------

inline void CSPSRecord::GetHalfSize(vector2& Out) const
{
	Out.x = (GlobalBox.Max.x - GlobalBox.Min.x) * 0.5f;
	Out.y = (GlobalBox.Max.z - GlobalBox.Min.z) * 0.5f;
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
	n_assert_dbg(false);
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

inline void CSPS::RemoveAlwaysVisibleByValue(CSPSRecord* Object)
{
	TObjTraits::GetPtr(Object)->GetQuadTreeNode()->RemoveByValue(Object);
}
//---------------------------------------------------------------------

inline void CSPS::RemoveAlwaysVisibleByHandle(CHandle Handle)
{
	TObjTraits::GetPtr(*Handle)->GetQuadTreeNode()->RemoveByHandle(Handle);
}
//---------------------------------------------------------------------

//!!!need masks like ShadowCaster, ShadowReceiver for shadow camera etc!
void SPSCollectVisibleObjects(CSPSNode* pNode, const matrix44& ViewProj, const CAABB& SceneBBox, CArray<CRenderObject*>* OutObjects, CArray<CLight*>* OutLights = NULL, EClipStatus Clip = Clipped);

}

#endif
