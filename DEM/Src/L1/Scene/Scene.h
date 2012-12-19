#pragma once
#ifndef __DEM_L1_SCENE_H__
#define __DEM_L1_SCENE_H__

#include <Scene/SceneNode.h>
#include <Scene/Camera.h>
#include <Scene/SPS.h>

// 3D scene with node hierarchy and volume subdivided to optimize spatial requests

namespace Scene
{
class CLight;

class CScene: public Core::CRefCounted
{
private:

	PSceneNode				RootNode;
	nArray<PSceneNode>		OwnedNodes;

	vector4					AmbientLight;
	//Fog settings
	//???shadow settings?

	bbox3					SceneBBox;
	PCamera					CurrCamera; //???or store externally?

	nArray<CRenderObject*>	VisibleObjects;
	nArray<CLight*>			VisibleLights;

	//!!!need masks like ShadowCaster, ShadowReceiver for shadow camera etc!
	void SPSCollectVisibleObjects(CSPSNode* pNode, const matrix44& ViewProj, nArray<CRenderObject*>* OutObjects, nArray<CLight*>* OutLights = NULL, EClipStatus Clip = Clipped);

public:

	CSPS				SPS;			// Spatial partitioning structure

	CScene(): OwnedNodes(0, 32), AmbientLight(0.2f, 0.2f, 0.2f, 1.f) { OwnedNodes.SetFlags(nArray<PSceneNode>::DoubleGrowSize); }
	~CScene() { Clear(); }

	void		Init(const bbox3& Bounds);
	void		Activate();
	void		Deactivate();
	void		Clear();

	void		CreateDefaultCamera();

	//!!!can add global lights to separate array if necessary!
	void		AddVisibleLight(CLight& Light) { VisibleLights.Append(&Light); }

	bool		Render(PCamera Camera = NULL, CStrID FrameShaderID = CStrID::Empty);

	void		OwnNode(PSceneNode Node);
	bool		FreeNode(PSceneNode Node);
	PSceneNode	GetNode(LPCSTR Path, bool Create = false) { return (Path && *Path) ? RootNode->GetChild(Path, Create) : RootNode; }
	CSceneNode&	GetRootNode() const { return *RootNode.get(); }
	CCamera*	GetCurrCamera() const { return CurrCamera.get_unsafe(); }
};

typedef Ptr<CScene> PScene;

// Scene-owned nodes die only on scene destruction, whereas object-owned nodes die on owner object destruction
inline void CScene::OwnNode(PSceneNode Node)
{
	if (Node.isvalid() && !Node->IsOwnedByScene())
	{
		OwnedNodes.Append(Node);
		Node->Flags.Set(CSceneNode::OwnedByScene);
	}
}
//---------------------------------------------------------------------

inline bool CScene::FreeNode(PSceneNode Node)
{
	if (!Node.isvalid() || !Node->IsOwnedByScene() || !OwnedNodes.RemoveByValue(Node)) FAIL;
	Node->Flags.Clear(CSceneNode::OwnedByScene);
	OK;
}
//---------------------------------------------------------------------

}

#endif
