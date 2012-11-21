#pragma once
#ifndef __DEM_L1_SCENE_MESH_H__
#define __DEM_L1_SCENE_MESH_H__

#include <Scene/SceneNodeAttr.h>

// Mesh is a scene node attribute representing a visible shape.
// Mesh attribute references VB & IB resources, stores vertex and index range,
// material, its parameters and texture refs.

//!!!it is good to have attr for each mesh group, so separate visibility test
// is performed and shader sorting simplifies

class bbox3;

namespace Scene
{
struct CSPSRecord;

class CMesh: public CSceneNodeAttr
{
	DeclareRTTI;

public:

	// PVertexBuffer VB; // Format inside
	// PIndexBuffer IB;
	// Group (FirstVertex, VertexCount, FirstIndex, IndexCount, Topology (primtype), AABB) //???VB + IB + Group to resource?
	// Material (shader ref with name-to-handle for vars inside)
	// PTexture Textures[];

	// ERenderFlag: ShadowCaster, ShadowReceiver, DoOcclusionCulling

	CSPSRecord*	pSPSRecord;

	CMesh(): pSPSRecord(NULL) {}

	virtual void	UpdateTransform(CScene& Scene);
	void			GetBox(bbox3& OutBox) const;
};

typedef Ptr<CMesh> PMesh;

inline void CMesh::GetBox(bbox3& OutBox) const
{
	// If local params changed, recompute AABB
	// If transform of host node changed, update global space AABB (rotate, scale)
}
//---------------------------------------------------------------------

}

#endif
