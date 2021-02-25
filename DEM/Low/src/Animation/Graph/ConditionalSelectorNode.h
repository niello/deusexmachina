#pragma once
#include <Animation/Graph/SelectorNodeBase.h>
#include <Data/StringID.h>

// Animation graph node that selects a subnode based on a boolean value

//!!!TODO: lazy eval and then cache conditions in a controller!

namespace DEM::Anim
{
using PConditionalSelectorNode = std::unique_ptr<class CConditionalSelectorNode>;

class CConditionalSelectorNode : public CSelectorNodeBase
{
protected:

	CStrID               _ParamID;
	UPTR                 _ParamIndex = INVALID_INDEX; // Cached for fast access

	CVariant             _TrueVariant;
	CVariant             _FalseVariant;

	virtual CVariant* SelectVariant(CAnimationUpdateContext& Context) override;

public:

	CConditionalSelectorNode(CStrID ParamID);

	virtual void Init(CAnimationInitContext& Context) override;
};

}
