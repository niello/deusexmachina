#pragma once
#ifndef __DEM_L2_GFX_LEVEL_H__
#define __DEM_L2_GFX_LEVEL_H__

#include <Core/RefCounted.h>
#include <Gfx/Entity.h>
#include <kernel/nprofiler.h>

// The CLevel class contains all pCell and pEntity objects in a level and is responsible
// for rendering them efficiently.

//???!!!IMPLEMENT OCTREE INSTEAD OF QUADTREE?!

namespace Graphics
{
class CEntity;
class CLightEntity;
class CCameraEntity;

class CLevel: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CLevel);

private:

	bbox3				Box;
	CGfxQT				QuadTree;

	Ptr<CCameraEntity>	DefaultCamera;
	Ptr<CCameraEntity>	CurrCamera;

	void QTNodeClearLinks(CGfxQT::CNode* pNode, ELinkType LinkType);
	void QTNodeUpdateLinks(CGfxQT::CNode* pNode, CEntity& Observer, EEntityType ObservedType, ELinkType LinkType, EClipStatus Clip = InvalidClipStatus);
	void QTNodeRenderDebug(CGfxQT::CNode* pNode);

	PROFILER_DECLARE(profFindVisibleLights);
	PROFILER_DECLARE(profFindLitObjects);
	PROFILER_DECLARE(profFindVisibleObjects);
	PROFILER_DECLARE(profCameraRenderBefore);
	PROFILER_DECLARE(profCameraRender);
	PROFILER_DECLARE(profClearLinks);

	//!!!STATIC!
#if __NEBULA_STATS__
	void AddNumVisibleEntities(EEntityType ObservedType, ELinkType LinkType, int incr);
	void AddNumVisibleCells(ELinkType LinkType, int incr);
#endif

public:

	CLevel();
	//virtual ~CLevel();

	void			Init(const bbox3& LevelBox, uchar QuadTreeDepth);

	bool			BeginRender();
	void			Render();
	void			RenderDebug();
	//void			EndRender() {}

	void			AttachEntity(CEntity* pEntity);
	void			UpdateEntityLocation(CEntity* pEntity);
	void			RemoveEntity(CEntity* pEntity);

	void			SetCamera(CCameraEntity* pCamera);
	CCameraEntity*	GetCamera() const { return CurrCamera.get_unsafe(); }
};

RegisterFactory(CLevel);

}

#endif
