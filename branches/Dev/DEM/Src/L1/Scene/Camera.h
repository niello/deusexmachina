#pragma once
#ifndef __DEM_L1_SCENE_CAMERA_H__
#define __DEM_L1_SCENE_CAMERA_H__

#include <Scene/SceneNodeAttr.h>

// Camera is a scene node attribute describing camera properties.
// Note - W and H are necessary for orthogonal projection matrix,
// aspect ratio for a prespective projection can be calculated as W / H.

//!!!cache calculated proj and invproj matrices!
//???cache view matrix? camera transform is InvView

namespace Scene
{

class CCamera: public CSceneNodeAttr
{
	DeclareRTTI;

public:

	float	FOV;
	float	Width;
	float	Height;
	float	NearPlane;
	float	FarPlane;

	//background color
	//???clip planes computing?

	virtual void UpdateTransform(CScene& Scene);
};

typedef Ptr<CCamera> PCamera;

}

#endif
