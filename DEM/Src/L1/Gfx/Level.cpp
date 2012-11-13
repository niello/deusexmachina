#include "Level.h"

#include <Gfx/CameraEntity.h>
#include <Gfx/LightEntity.h>
#include <Gfx/ShapeEntity.h>
#include <Gfx/GfxServer.h>
#include <gfx2/ngfxserver2.h>
#include <particle/nparticleserver.h>
#include <scene/nsceneserver.h>

namespace Graphics
{
ImplementRTTI(Graphics::CLevel, Core::CRefCounted);

CLevel::CLevel()
{
	DefaultCamera.Create();

	PROFILER_INIT(profFindVisibleLights, "profMangaGfxFindVisibleLights");
	PROFILER_INIT(profFindLitObjects, "profMangaGfxFindLitObjects");
	PROFILER_INIT(profFindVisibleObjects, "profMangaGfxFindVisibleObjects");
	PROFILER_INIT(profCameraRenderBefore, "profMangaGfxCameraRenderBefore");
	PROFILER_INIT(profCameraRender, "profMangaGfxCameraRender");
	PROFILER_INIT(profClearLinks, "profMangaGfxClearLinks");
}
//---------------------------------------------------------------------

void CLevel::Init(const bbox3& LevelBox, uchar QuadTreeDepth)
{
	Box = LevelBox;
	vector3 Center = LevelBox.center();
	vector3 Size = LevelBox.size();
	QuadTree.Build(Center.x, Center.z, Size.x, Size.z, QuadTreeDepth);
	SetCamera(DefaultCamera);
}
//---------------------------------------------------------------------

void CLevel::SetCamera(CCameraEntity* pCamera)
{
	if (CurrCamera.isvalid()) RemoveEntity(CurrCamera);
	CurrCamera = pCamera;
	if (pCamera) AttachEntity(CurrCamera);
}
//---------------------------------------------------------------------

void CLevel::AttachEntity(CEntity* pEntity)
{
	n_assert(!pEntity->pLevel);
	pEntity->MaxVisibleDistance = GfxSrv->GetDistThreshold(pEntity->GetRTTI());
	pEntity->MinVisibleSize = GfxSrv->GetSizeThreshold(pEntity->GetRTTI());
	pEntity->pLevel = this;
	pEntity->Activate();
	QuadTree.AddObject(pEntity);
}
//---------------------------------------------------------------------

void CLevel::UpdateEntityLocation(CEntity* pEntity)
{
	n_assert(pEntity->pLevel == this && pEntity->pQTNode);
	QuadTree.UpdateObject(pEntity); //!!!Chase camera updates every frame even without tfm change!
}
//---------------------------------------------------------------------

void CLevel::RemoveEntity(CEntity* pEntity)
{
	n_assert(pEntity->pLevel == this);
	pEntity->Deactivate();
	pEntity->pLevel = NULL;
	QuadTree.RemoveObject(pEntity);
}
//---------------------------------------------------------------------

bool CLevel::BeginRender()
{
	//n_assert(QuadTree.IsInitialized());
	n_assert(CurrCamera.isvalid());

#if __NEBULA_STATS__
	//numShapesVisible->SetValue<int>(0);
	//numLightsVisible->SetValue<int>(0);
	//numShapesLit->SetValue<int>(0);
	//numCellsVisitedCamera->SetValue<int>(0);
	//numCellsVisitedLight->SetValue<int>(0);
	//numCellsOutsideCamera->SetValue<int>(0);
	//numCellsOutsideLight->SetValue<int>(0);
	//numCellsVisibleCamera->SetValue<int>(0);
	//numCellsVisibleLight->SetValue<int>(0);
#endif

	PROFILER_START(profClearLinks);
	QTNodeClearLinks(QuadTree.GetRootNode(), CameraLink);
	QTNodeClearLinks(QuadTree.GetRootNode(), LightLink);
	PROFILER_STOP(profClearLinks);

	// first, get all visible light sources
	// NOTE: at first glance it look bad to traverse twice through
	// the tree for camera visibility (first for lights, then for
	// shapes). Unfortunately this is necessary because entities
	// need to compute their shadow bounding boxes for view culling,
	// so they need their light links updated first!
	PROFILER_START(profFindVisibleLights);
	QTNodeUpdateLinks(QuadTree.GetRootNode(), *CurrCamera, GFXLight, CameraLink);
	PROFILER_STOP(profFindVisibleLights);

	// for each visible light source, gather entities lit by this light source
	PROFILER_START(profFindLitObjects);
	for (int i = 0; i < CurrCamera->GetNumLinks(CameraLink); i++)
	{
		CEntity* pLightEntity = CurrCamera->GetLinkAt(CameraLink, i);
		n_assert(pLightEntity->GetType() == GFXLight);
		QTNodeUpdateLinks(QuadTree.GetRootNode(), *pLightEntity, GFXShape, LightLink);
	}
	PROFILER_STOP(profFindLitObjects);

	// gather shapes visible from the camera
	PROFILER_START(profFindVisibleObjects);
	QTNodeUpdateLinks(QuadTree.GetRootNode(), *CurrCamera, GFXShape, CameraLink);
	PROFILER_STOP(profFindVisibleObjects);

	return nSceneServer::Instance()->BeginScene(CurrCamera->GetTransform());
}
//---------------------------------------------------------------------

// Entity links MUST be cleared before calling Cell::UpdateLinks()
void CLevel::QTNodeClearLinks(CGfxQT::CNode* pNode, ELinkType LinkType)
{
	if (!pNode->GetTotalObjCount()) return;

	for (int i = 0; i < GFXEntityNumTypes; i++)
	{
		CGfxEntityListSet::CElement* pCurr = pNode->Data.Entities.GetHead(EEntityType(i));
		for (; pCurr; pCurr = pCurr->GetSucc())
			pCurr->Object->ClearLinks(LinkType);
	}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
			QTNodeClearLinks(pNode->GetChild(i), LinkType);
}
//---------------------------------------------------------------------

/*	Recursively creates links between observer and observed objects based
	on visibility status (a link will be created between an observer and
	an pEntity visible to that observer). This method is used to
	create links between camera and shape entities, or light and shape
	entities (just tell the method what you want). */
// Recursively gather all entities of a given type which are visible by a observer entity.
// A link will be established between the observer and observed entity.
// This method is actually the core of the whole visibility detection!
void CLevel::QTNodeUpdateLinks(CGfxQT::CNode* pNode, CEntity& Observer, EEntityType ObservedType,
							   ELinkType LinkType, EClipStatus Clip)
{
	//!!!if (!EntityCountByType[ObservedType]) return;
	if (!pNode->GetTotalObjCount()) return;

	//if (LinkType == LightLink)
	//	numCellsVisitedLight->SetValue<int>(numCellsVisitedLight->GetValue<int>() + 1);
	//else
	//	numCellsVisitedCamera->SetValue<int>(numCellsVisitedCamera->GetValue<int>() + 1);

	// if clip status unknown or clipped, get clip status of this cell against observer entity
	if (Clip == InvalidClipStatus || Clip == Clipped)
	{
		bbox3 BBox;
		pNode->GetBounds(BBox);
		BBox.vmin.y = Box.vmin.y;
		BBox.vmax.y = Box.vmax.y;
		Clip = Observer.GetBoxClipStatus(BBox);
	}

	if (Clip == Outside)
	{
		//if (LinkType == LightLink)
		//	numCellsOutsideLight->SetValue<int>(numCellsOutsideLight->GetValue<int>() + 1);
		//else
		//	numCellsOutsideCamera->SetValue<int>(numCellsOutsideCamera->GetValue<int>() + 1);

		return;
	}
	else if (Clip == Inside)
	{
		AddNumVisibleCells(LinkType, 1);

		// Cell completely inside, gather ALL contained shape entities
		CGfxEntityListSet::CElement* pCurr = pNode->Data.Entities.GetHead(ObservedType);
		for (; pCurr; pCurr = pCurr->GetSucc())
		{
			CRenderableEntity* pEntity = (CRenderableEntity*)pCurr->Object.get();
			if (pEntity->GetVisible() && pEntity->TestLODVisibility())
			{
				Observer.AddLink(LinkType, pEntity);
				pEntity->AddLink(LinkType, &Observer);

				if (LinkType == CameraLink && ObservedType == GFXShape)
				{
					((CShapeEntity*)pEntity)->SetRenderFlag(nRenderContext::ShadowVisible, true);
					((CShapeEntity*)pEntity)->SetRenderFlag(nRenderContext::ShapeVisible, true);
				}
				AddNumVisibleEntities(ObservedType, LinkType, 1);
			}
		}
	}
	else
	{
		AddNumVisibleCells(LinkType, 1);

		// Cell partially inside, check each entity individually
		CGfxEntityListSet::CElement* pCurr = pNode->Data.Entities.GetHead(ObservedType);
		for (; pCurr; pCurr = pCurr->GetSucc())
		{
			CRenderableEntity* pEntity = (CRenderableEntity*)pCurr->Object.get();
			if (pEntity->GetVisible() && pEntity->TestLODVisibility())
			{
				// check against extruded shadow or canonical bounding box, depending on link Type
				bool SetRenderCtxHints = (CameraLink == LinkType && GFXShape == ObservedType);
				const bbox3& EntityBox = SetRenderCtxHints ? ((CShapeEntity*)pEntity)->GetShadowBox() : pEntity->GetBox();

				if (Observer.GetBoxClipStatus(EntityBox) != Outside)
				{
					Observer.AddLink(LinkType, pEntity);
					pEntity->AddLink(LinkType, &Observer);
					if (SetRenderCtxHints)
					{
						((CShapeEntity*)pEntity)->SetRenderFlag(nRenderContext::ShadowVisible, true);
						((CShapeEntity*)pEntity)->SetRenderFlag(nRenderContext::ShapeVisible,
							Observer.GetBoxClipStatus(pEntity->GetBox()) != Outside);
					}
					AddNumVisibleEntities(ObservedType, LinkType, 1);
				}
			}
		}
	}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
			QTNodeUpdateLinks(pNode->GetChild(i), Observer, ObservedType, LinkType, Clip);
}
//---------------------------------------------------------------------

void CLevel::Render()
{
	PROFILER_START(profCameraRenderBefore);
	CurrCamera->OnRenderBefore();
	PROFILER_STOP(profCameraRenderBefore);

	PROFILER_START(profCameraRender);
	CurrCamera->RenderLinks();
	PROFILER_STOP(profCameraRender);

	nSceneServer::Instance()->EndScene();
	nSceneServer::Instance()->RenderScene();
}
//---------------------------------------------------------------------

void CLevel::RenderDebug()
{
	nGfxServer2::Instance()->BeginShapes();
	QTNodeRenderDebug(QuadTree.GetRootNode());
	nGfxServer2::Instance()->EndShapes();
}
//---------------------------------------------------------------------

void CLevel::QTNodeRenderDebug(CGfxQT::CNode* pNode)
{
	CGfxEntityListSet::CElement* pCurr = pNode->Data.Entities.GetHead(GFXShape);
	for (; pCurr; pCurr = pCurr->GetSucc())
		pCurr->Object->RenderDebug();

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
			QTNodeRenderDebug(pNode->GetChild(i));
}
//---------------------------------------------------------------------

#if __NEBULA_STATS__
void CLevel::AddNumVisibleEntities(EEntityType ObservedType, ELinkType LinkType, int incr)
{
	//if (ObservedType == GFXLight)
	//	numLightsVisible->SetValue<int>(numLightsVisible->GetValue<int>() + incr);
	//else
	//{
	//	if (LinkType == LightLink) numShapesLit->SetValue<int>(numShapesLit->GetValue<int>() + incr);
	//	else numShapesVisible->SetValue<int>(numShapesVisible->GetValue<int>() + incr);
	//}
}
//---------------------------------------------------------------------

void CLevel::AddNumVisibleCells(ELinkType LinkType, int incr)
{
	//if (LinkType == LightLink)
	//	numCellsVisibleLight->SetValue<int>(numCellsVisibleLight->GetValue<int>() + incr);
	//else
	//	numCellsVisibleCamera->SetValue<int>(numCellsVisibleCamera->GetValue<int>() + incr);
}
//---------------------------------------------------------------------
#endif

} // namespace Graphics
