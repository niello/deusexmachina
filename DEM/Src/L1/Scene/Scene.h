#pragma once
#ifndef __DEM_L1_SCENE_H__
#define __DEM_L1_SCENE_H__

#include <Scene/SceneNode.h>
#include <mathlib/bbox.h>

// 3D scene with node hierarchy and volume subdivided to optimize spatial requests

namespace Scene
{

class CScene: public Core::CRefCounted
{
private:

	PSceneNode			RootNode;

	vector4				AmbientLight;
	//Fog settings
	//???shadow settings?

	nArray<PSceneNode>	OwnedNodes;

	// Curr camera (node or attr or smth?)
	// Spatial partitioning structure (quadtree, octree or BVH)

public:

	CScene(): OwnedNodes(0, 32), AmbientLight(0.2f, 0.2f, 0.2f, 1.f) { OwnedNodes.SetFlags(nArray<PSceneNode>::DoubleGrowSize); }
	~CScene() { Clear(); }

	void		Init(const bbox3& Bounds);
	void		Activate();
	void		Deactivate();
	void		Clear();

	void		OwnNode(PSceneNode Node);
	bool		FreeNode(PSceneNode Node);
	PSceneNode	GetNode(LPCSTR Path, bool Create = false) { return (Path && *Path) ? RootNode->GetChild(Path, Create) : RootNode; }
	CSceneNode&	GetRootNode() const { return *RootNode.get(); }
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
	if (!Node.isvalid() || !Node->IsOwnedByScene() || !OwnedNodes.EraseElement(Node)) FAIL;
	Node->Flags.Clear(CSceneNode::OwnedByScene);
	OK;
}
//---------------------------------------------------------------------

}

#endif
