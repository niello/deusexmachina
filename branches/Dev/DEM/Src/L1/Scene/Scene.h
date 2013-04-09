#pragma once
#ifndef __DEM_L1_SCENE_H__
#define __DEM_L1_SCENE_H__

#include <Scene/SceneNode.h>
#include <Scene/Camera.h>
#include <Scene/SPS.h>

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

public:

	CSPS				SPS;			// Spatial partitioning structure

	CScene(): AmbientLight(0.2f, 0.2f, 0.2f, 1.f) {  }
	~CScene() { Clear(); }

	void		Init(const bbox3& Bounds);
	void		Activate();
	void		Deactivate();
	void		Clear();

	//!!!can add global lights to separate array if necessary!
	void		AddVisibleObject(CRenderObject& Obj) { VisibleObjects.Append(&Obj); }
	void		AddVisibleLight(CLight& Light) { VisibleLights.Append(&Light); }

	bool		Render(PCamera Camera, Render::CFrameShader& FrameShader);

	CSceneNode*	GetNode(LPCSTR Path, bool Create = false) { return (Path && *Path) ? RootNode->GetChild(Path, Create) : RootNode.get_unsafe(); }
	CSceneNode&	GetRootNode() { return *RootNode.get_unsafe(); }
	void		SetMainCamera(CCamera* pNewCamera) { MainCamera = pNewCamera ? pNewCamera : DefaultCamera; }
	CCamera*	GetMainCamera() const { return MainCamera.get_unsafe(); }
};

typedef Ptr<CScene> PScene;

}

#endif
