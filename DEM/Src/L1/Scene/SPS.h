#pragma once
#ifndef __DEM_L1_SCENE_SPS_H__
#define __DEM_L1_SCENE_SPS_H__

#include <Scene/Mesh.h>
#include <Scene/Light.h>
#include <Scene/SceneNode.h>
#include <Data/QuadTree.h>
#include <kernel/ndebug.h>

// Scene spatial partitioning structure stuff

namespace Scene
{
struct CSPSRecord;

struct CSPSCell
{
	typedef CSPSRecord CElement;

	nArray<CElement>	Meshes;
	nArray<CElement>	Lights;

	CElement*	Add(CSPSRecord* const& Object);
	bool		Remove(CSPSRecord* const& Object); // By value
	void		RemoveElement(CElement* pElement); // By iterator
};

typedef Data::CQuadTree<CSPSRecord*, CSPSCell> CSPS;
typedef CSPS::CNode CSPSNode;

class CSceneNodeAttr;

struct CSPSRecord
{
	//CSceneNodeAttr&	Attr; // nArray<T>::Construct needs default constructor, change it to copy constructor!
	CSceneNodeAttr*	pAttr;
	bbox3			GlobalBox; // AABB in global space. Get position from scene node.
	CSPSNode*		pSPSNode;

	CSPSRecord(): pAttr(NULL), pSPSNode(NULL) {}

	bool		IsMesh() const { return pAttr->IsA(CMesh::RTTI); }
	bool		IsLight() const { return pAttr->IsA(CLight::RTTI); }

	CSPSRecord*	GetObject() { return this; }
	void		GetCenter(vector2& Out) const;
	void		GetHalfSize(vector2& Out) const;
	CSPSNode*	GetQuadTreeNode() const { return pSPSNode; }
	void		SetQuadTreeNode(CSPSNode* pNode) { pSPSNode = pNode; }

	bool operator ==(const CSPSRecord& Other) const { return pAttr == Other.pAttr; }
	bool operator !=(const CSPSRecord& Other) const { return pAttr != Other.pAttr; }
};

inline void CSPSRecord::GetCenter(vector2& Out) const
{
	n_assert(pAttr && pAttr->GetNode());
	const vector3& Pos = pAttr->GetNode()->GetWorldMatrix().pos_component();
	Out.x = Pos.x;
	Out.y = Pos.z;
}
//---------------------------------------------------------------------

inline void CSPSRecord::GetHalfSize(vector2& Out) const
{
	Out.x = (GlobalBox.vmax.x - GlobalBox.vmin.x) * 0.5f;
	Out.y = (GlobalBox.vmax.z - GlobalBox.vmin.z) * 0.5f;
}
//---------------------------------------------------------------------

inline CSPSCell::CElement* CSPSCell::Add(CSPSRecord* const& Object)
{
	n_assert(Object);
	if (Object->IsMesh()) return &Meshes.Append(*Object);
	if (Object->IsLight()) return &Lights.Append(*Object);
	n_assert_dbg(false);
	return NULL;
}
//---------------------------------------------------------------------

// Remove by value
inline bool CSPSCell::Remove(CSPSRecord* const& Object)
{
	n_assert(Object);
	if (Object->IsMesh()) return Meshes.EraseElement(*Object);
	if (Object->IsLight()) return Lights.EraseElement(*Object);
	FAIL;
}
//---------------------------------------------------------------------

// Remove by iterator
inline void CSPSCell::RemoveElement(CSPSCell::CElement* pElement)
{
	if (!pElement) return;
	if (pElement->IsMesh()) Meshes.Erase(pElement);
	if (pElement->IsLight()) Lights.Erase(pElement);
}
//---------------------------------------------------------------------

}

#endif
