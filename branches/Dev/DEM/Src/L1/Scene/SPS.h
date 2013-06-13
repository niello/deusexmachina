#pragma once
#ifndef __DEM_L1_SCENE_SPS_H__
#define __DEM_L1_SCENE_SPS_H__

#include <Scene/RenderObject.h>
#include <Scene/Light.h>
#include <Scene/SceneNode.h>
#include <Data/QuadTree.h>

// Scene spatial partitioning structure stuff

namespace Scene
{
struct CSPSRecord;

struct CSPSCell
{
	typedef CSPSRecord* CElement;

	nArray<CElement> Objects;
	nArray<CElement> Lights;

	CElement*	Add(CSPSRecord* const & Object);
	bool		RemoveByValue(CSPSRecord* const & Object); // By value
	void		RemoveElement(CElement* pElement) { n_error("No persistent handles for arrays!"); } // By CIterator
};

typedef Data::CQuadTree<CSPSRecord*, CSPSCell> CSPS;
typedef CSPS::CNode CSPSNode;

class CNodeAttribute;

struct CSPSRecord
{
	CNodeAttribute&	Attr;
	bbox3			GlobalBox;
	CSPSNode*		pSPSNode;

	CSPSRecord(CNodeAttribute& NodeAttr): Attr(NodeAttr), pSPSNode(NULL) {} 
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
	Out.x = (GlobalBox.vmax.x - GlobalBox.vmin.x) * 0.5f;
	Out.y = (GlobalBox.vmax.z - GlobalBox.vmin.z) * 0.5f;
}
//---------------------------------------------------------------------

// NB: no persistent handle for arrays
inline CSPSCell::CElement* CSPSCell::Add(CSPSRecord* const & Object)
{
	if (Object->IsRenderObject())
	{
		Objects.Append(Object);
		return NULL;
	}
	if (Object->IsLight())
	{
		Lights.Append(Object);
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

}

#endif
