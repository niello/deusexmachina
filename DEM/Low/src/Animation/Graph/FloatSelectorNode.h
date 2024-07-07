#pragma once
#include <Animation/Graph/SelectorNodeBase.h>
#include <Data/StringID.h>
#include <Data/VarStorage.h> // for HVar

// Animation graph node that selects a subnode based on a float value being in range [From; To)

namespace DEM::Anim
{
using PFloatSelectorNode = std::unique_ptr<class CFloatSelectorNode>;

class CFloatSelectorNode : public CSelectorNodeBase
{
protected:

	CStrID               _ParamID;
	HVar                 _ParamHandle; // Cached for fast access

	virtual CVariant* SelectVariant(CAnimationUpdateContext& Context) override;

public:

	CFloatSelectorNode(CStrID ParamID);

	virtual void Init(CAnimationInitContext& Context) override;
};

}
