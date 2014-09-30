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

	// Square threshold to child ID map.
	// Use FLT_MAX as a key if the last LOD must be active at any distance.
	// Use CStrID::Empty as a value to disable all children at some distance.
	CDict<float, CStrID> SqThresholds;

public:

	virtual bool LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual void Update(const vector3* pCOIArray, DWORD COICount);
};

typedef Ptr<CLODGroup> PLODGroup;

}

#endif
