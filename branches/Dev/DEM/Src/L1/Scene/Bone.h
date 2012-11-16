#pragma once
#ifndef __DEM_L1_SCENE_BONE_H__
#define __DEM_L1_SCENE_BONE_H__

#include <Scene/SceneNodeAttr.h>

// Skeletal animation bone. Manages skinning matrix, which is used by shader through node's shader params.

namespace Scene
{
struct CSPSRecord;

class CBone: public CSceneNodeAttr
{
	DeclareRTTI;

protected:

	matrix44	InvBindPose;
	matrix44	SkinMatrix;

public:

	//???update skin matrix only on preparing to render?
	// SkinMatrix = InvBindPose * Node->WorldTfm; //!!!use mult_simple!
	// OnAttach - provide pointer to shader params
	// OnDetach - remove/clear pointer

	//virtual void	UpdateTransform(CScene& Scene);

	//!!!Can be useful for ragdolls!
	//void			GetBox(bbox3& OutBox) const;
};

typedef Ptr<CBone> PBone;

}

#endif
