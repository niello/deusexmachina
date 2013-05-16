#pragma once
#ifndef __DEM_L1_SCENE_H__
#define __DEM_L1_SCENE_H__

#include <Scene/SceneNode.h>
#include <Scene/Camera.h>
#include <Scene/SPS.h>
#include <Events/Events.h>
#include <Events/Subscription.h>

// 3D scene with node hierarchy and volume subdivided to optimize spatial requests

namespace Render
{
	class CFrameShader;
}

namespace Scene
{
class CLight;

class CScene: public Core::CRefCounted
{
private:

	PSceneNode				RootNode;
	bbox3					SceneBBox;
	PCamera					DefaultCamera;
	PCamera					MainCamera;

	vector4					AmbientLight;
	//Fog settings
	//???shadow settings?

	nArray<CRenderObject*>	VisibleObjects;	//PERF: //???use buckets instead? may be it will be faster
	nArray<CLight*>			VisibleLights;

	//!!!need masks like ShadowCaster, ShadowReceiver for shadow camera etc!
	void SPSCollectVisibleObjects(CSPSNode* pNode, const matrix44& ViewProj, nArray<CRenderObject*>* OutObjects, nArray<CLight*>* OutLights = NULL, EClipStatus Clip = Clipped);

	DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnRenderDeviceReset);

public:

	CSPS				SPS;						// Spatial partitioning structure
	bool				AutoAdjustCameraAspect;

	CScene(): AmbientLight(0.2f, 0.2f, 0.2f, 1.f), AutoAdjustCameraAspect(true) {  }
	~CScene() { Clear(); }

	void		Init(const bbox3& Bounds, DWORD SPSHierarchyDepth);
	void		Activate();
	void		Deactivate();
	void		Clear();
	void		ClearVisibleLists() { VisibleObjects.Clear(); VisibleLights.Clear(); }

	//!!!can add global lights to separate array if necessary!
	void		AddVisibleObject(CRenderObject& Obj) { VisibleObjects.Append(&Obj); }
	void		AddVisibleLight(CLight& Light) { VisibleLights.Append(&Light); }

	bool		Render(PCamera Camera, Render::CFrameShader& FrameShader);

	CSceneNode*	GetNode(LPCSTR Path, bool Create = false) { return (Path && *Path) ? RootNode->GetChild(Path, Create) : RootNode.GetUnsafe(); }
	CSceneNode&	GetRootNode() { return *RootNode.GetUnsafe(); }
	void		SetMainCamera(CCamera* pNewCamera);
	CCamera&	GetMainCamera() const { return *MainCamera; }
};

typedef Ptr<CScene> PScene;

}

#endif
