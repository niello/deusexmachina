#include <Scene/SPS.h>

namespace Scene
{

void CSPS::QueryObjectsInsideFrustum(const matrix44& ViewProj, CArray<void*>& OutObjects) const
{
	if (OversizedObjects.GetCount())
		OutObjects.AddArray(OversizedObjects);

	CSPSNode* pRootNode = QuadTree.GetRootNode();
	if (pRootNode && pRootNode->GetTotalObjCount())
		QueryObjectsInsideFrustum(pRootNode, ViewProj, OutObjects, Clipped);
}
//---------------------------------------------------------------------

void CSPS::QueryObjectsInsideFrustum(CSPSNode* pNode, const matrix44& ViewProj, CArray<void*>& OutObjects, EClipStatus Clip) const
{
	n_assert_dbg(pNode && pNode->GetTotalObjCount() && Clip != Outside);

	if (Clip == Clipped)
	{
		CAABB NodeBox;
		pNode->GetBounds(NodeBox); //!!!can pass node box as arg and calculate for children, this will save some calculations!
		NodeBox.Min.y = SceneMinY;
		NodeBox.Max.y = SceneMaxY;
		Clip = NodeBox.GetClipStatus(ViewProj);
		if (Clip == Outside) return;
	}

	if (pNode->Data.Objects.GetCount())
	{
		CArray<CSPSRecord*>::CIterator ItObj = pNode->Data.Objects.Begin();
		if (Clip == Inside)
		{
			CArray<void*>::CIterator ppObj = OutObjects.Reserve(pNode->Data.Objects.GetCount());
			for (; ItObj != pNode->Data.Objects.End(); ++ItObj, ++ppObj)
				*ppObj = (*ItObj)->pUserData;
		}
		else // Clipped
		{
			//???test against global AABB or transform to model space and test against local AABB which fits tighter?
			for (; ItObj != pNode->Data.Objects.End(); ++ItObj)
				if ((*ItObj)->GlobalBox.GetClipStatus(ViewProj) != Outside)
					OutObjects.Add((*ItObj)->pUserData);
		}
	}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; ++i)
		{
			CSPSNode* pChildNode = pNode->GetChild(i);
			if (pChildNode->GetTotalObjCount())
				QueryObjectsInsideFrustum(pChildNode, ViewProj, OutObjects, Clip);
		}
}
//---------------------------------------------------------------------

}
