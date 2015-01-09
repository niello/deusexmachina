#include "SPS.h"

namespace Render
{

void CSPS::QueryVisibleObjectsAndLights(const matrix44& ViewProj, CArray<CRenderObject*>* OutObjects, CArray<CLight*>* OutLights) const
{
	if (!OutObjects && !OutLights) return;

	if (OutObjects && AlwaysVisibleObjects.GetCount())
		OutObjects->AddArray(AlwaysVisibleObjects);

	if (OutLights && AlwaysVisibleLights.GetCount())
		OutLights->AddArray(AlwaysVisibleLights);

	CSPSNode* pRootNode = QuadTree.GetRootNode();
	if (pRootNode && pRootNode->GetTotalObjCount())
		QueryVisibleObjectsAndLights(pRootNode, ViewProj, OutObjects, OutLights, Clipped);
}
//---------------------------------------------------------------------

void CSPS::QueryVisibleObjectsAndLights(CSPSNode* pNode, const matrix44& ViewProj, CArray<CRenderObject*>* OutObjects, CArray<CLight*>* OutLights, EClipStatus Clip) const
{
	n_assert_dbg(pNode && pNode->GetTotalObjCount() && (OutObjects || OutLights) && Clip != Outside);

	if (Clip == Clipped)
	{
		CAABB NodeBox;
		pNode->GetBounds(NodeBox);
		NodeBox.Min.y = SceneMinY;
		NodeBox.Max.y = SceneMaxY;
		Clip = NodeBox.GetClipStatus(ViewProj);
		if (Clip == Outside) return;
	}

	if (OutObjects && pNode->Data.Objects.GetCount())
	{
		CArray<CSPSRecord*>::CIterator ItObj = pNode->Data.Objects.Begin();
		if (Clip == Inside)
		{
			CArray<CRenderObject*>::CIterator ppObj = OutObjects->Reserve(pNode->Data.Objects.GetCount());
			for (; ItObj != pNode->Data.Objects.End(); ++ItObj, ++ppObj)
				*ppObj = (CRenderObject*)(*ItObj)->pUserData;
		}
		else // Clipped
		{
			//???test against global AABB or transform to model space and test against local AABB which fits tighter?
			for (; ItObj != pNode->Data.Objects.End(); ++ItObj)
				if ((*ItObj)->GlobalBox.GetClipStatus(ViewProj) != Outside)
					OutObjects->Add((CRenderObject*)(*ItObj)->pUserData);
		}
	}

	if (OutLights && pNode->Data.Lights.GetCount())
	{
		CArray<CSPSRecord*>::CIterator ItLight = pNode->Data.Lights.Begin();
		if (Clip == Inside)
		{
			CArray<CLight*>::CIterator ppLight = OutLights->Reserve(pNode->Data.Lights.GetCount());
			for (; ItLight != pNode->Data.Lights.End(); ++ItLight, ++ppLight)
				*ppLight = (CLight*)(*ItLight)->pUserData;
		}
		else // Clipped
		{
			//???test against global AABB or transform to model space and test against local AABB which fits tighter?
			for (; ItLight != pNode->Data.Lights.End(); ++ItLight)
				if ((*ItLight)->GlobalBox.GetClipStatus(ViewProj) != Outside)
					OutLights->Add((CLight*)(*ItLight)->pUserData);
		}
	}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; ++i)
		{
			CSPSNode* pChildNode = pNode->GetChild(i);
			if (pChildNode->GetTotalObjCount())
				QueryVisibleObjectsAndLights(pChildNode, ViewProj, OutObjects, OutLights, Clip);
		}
}
//---------------------------------------------------------------------

}
