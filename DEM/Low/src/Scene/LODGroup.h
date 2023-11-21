#pragma once
#include <Scene/NodeAttribute.h>
#include <map>

// Level of detail group activates and deactivates child nodes of its node
// according to a distance to the Center Of Interest (COI). There may be multiple
// COIs, in that case the minimal distance is used.

//!!!can add switch (activator) attr and dynamic loading attr!
//load before activation, unload after deactivation, distances are farther (LoadRange > ActivateRange)

namespace Scene
{

class CLODGroup: public CNodeAttribute
{
	FACTORY_CLASS_DECL;

protected:

	// Square threshold to child ID map.
	// Use FLT_MAX as a key if the last LOD must be active at any distance.
	// Use CStrID::Empty as a value to disable all children at some distance.
	std::map<float, CStrID> SqThresholds;

public:

	virtual bool			LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual PNodeAttribute	Clone() override;
	virtual void			UpdateBeforeChildren(const rtm::vector4f* pCOIArray, UPTR COICount) override;
};

typedef Ptr<CLODGroup> PLODGroup;

}
