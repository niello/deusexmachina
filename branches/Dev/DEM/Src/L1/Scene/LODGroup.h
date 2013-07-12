#pragma once
#ifndef __DEM_L1_SCENE_LOD_GROUP_H__
#define __DEM_L1_SCENE_LOD_GROUP_H__

#include <Scene/NodeAttribute.h>
#include <Data/Dictionary.h>

// Level of detail group activates and deactivates child nodes of its node
// according to a distance to the main camera 

namespace Scene
{
struct CSPSRecord;

class CLODGroup: public CNodeAttribute
{
	__DeclareClass(CLODGroup);

protected:

	float						MinSqDistance;
	float						MaxSqDistance;
	CDict<float, CStrID>	SqThresholds;	// Square threshold to child ID map

public:

	CLODGroup(): MinSqDistance(0.f), MaxSqDistance(FLT_MAX) {}

	virtual bool LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual void Update();
};

typedef Ptr<CLODGroup> PLODGroup;

}

#endif
