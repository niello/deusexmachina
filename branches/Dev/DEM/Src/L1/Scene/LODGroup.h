#pragma once
#ifndef __DEM_L1_SCENE_LOD_GROUP_H__
#define __DEM_L1_SCENE_LOD_GROUP_H__

#include <Scene/SceneNodeAttr.h>
#include <util/ndictionary.h>

// Level of detail group activates and deactivates child nodes of its node
// according to a distance to the main camera 

namespace Scene
{
struct CSPSRecord;

class CLODGroup: public CSceneNodeAttr
{
	DeclareRTTI;

protected:

	float						MinSqDistance;
	float						MaxSqDistance;
	nDictionary<float, CStrID>	SqThresholds;	// Square threshold to child ID map

public:

	CLODGroup(): MinSqDistance(0.f), MaxSqDistance(FLT_MAX) {}

	virtual void UpdateTransform(CScene& Scene);
};

typedef Ptr<CLODGroup> PLODGroup;

}

#endif
