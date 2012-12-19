#pragma once
#ifndef __DEM_L1_SCENE_RENDER_OBJECT_H__
#define __DEM_L1_SCENE_RENDER_OBJECT_H__

#include <Scene/SceneNodeAttr.h>

// Base attribute class for any renderable scene objects, like
// regular models, particle systems, terrain patches etc.

namespace Scene
{

class CRenderObject: public CSceneNodeAttr
{
	DeclareRTTI;

public:
};

typedef Ptr<CRenderObject> PRenderObject;

}

#endif
