#include "SPS.h"

namespace Render
{

void SPSCollectVisibleObjects(CSPSNode* pNode, const matrix44& ViewProj, const CAABB& SceneBBox,
							  CArray<CRenderObject*>* OutObjects, CArray<CLight*>* OutLights, EClipStatus Clip)
{
	if (!pNode || !pNode->GetTotalObjCount() || (!OutObjects && !OutLights)) return;

	if (Clip == Clipped)
	{
		CAABB NodeBox;
		pNode->GetBounds(NodeBox);
		NodeBox.Min.y = SceneBBox.Min.y;
		NodeBox.Max.y = SceneBBox.Max.y;
		Clip = NodeBox.GetClipStatus(ViewProj);
		if (Clip == Outside) return;
	}

	if (OutObjects && pNode->Data.Objects.GetCount())
	{
		CArray<CSPSRecord*>::CIterator ItObj = pNode->Data.Objects.Begin();
		if (Clip == Inside)
		{
			CRenderObject** ppObj = OutObjects->Reserve(pNode->Data.Objects.GetCount());
			for (; ItObj != pNode->Data.Objects.End(); ++ItObj, ++ppObj)
				*ppObj = (CRenderObject*)&(*ItObj)->Attr;
		}
		else // Clipped
		{
			//???test against global box or transform to model space and test against local box?
			for (; ItObj != pNode->Data.Objects.End(); ++ItObj)
				if ((*ItObj)->GlobalBox.GetClipStatus(ViewProj) != Outside)
					OutObjects->Add((CRenderObject*)&(*ItObj)->Attr);
		}
	}

	if (OutLights && pNode->Data.Lights.GetCount())
	{
		CArray<CSPSRecord*>::CIterator ItLight = pNode->Data.Lights.Begin();
		if (Clip == Inside)
		{
			CLight** ppLight = OutLights->Reserve(pNode->Data.Lights.GetCount());
			for (; ItLight != pNode->Data.Lights.End(); ++ItLight, ++ppLight)
				*ppLight = (CLight*)&(*ItLight)->Attr;
		}
		else // Clipped
		{
			//???test against global box or transform to model space and test against local box?
			for (; ItLight != pNode->Data.Lights.End(); ++ItLight)
				if ((*ItLight)->GlobalBox.GetClipStatus(ViewProj) != Outside)
					OutLights->Add((CLight*)&(*ItLight)->Attr);
		}
	}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
			SPSCollectVisibleObjects(pNode->GetChild(i), ViewProj, SceneBBox, OutObjects, OutLights, Clip);
}
//---------------------------------------------------------------------

}
