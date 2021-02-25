#pragma once
#include <Animation/Graph/SelectorNodeBase.h>
#include <Data/StringID.h>

// Animation graph node that selects a subnode based on a string value

namespace DEM::Anim
{
using PStringSelectorNode = std::unique_ptr<class CStringSelectorNode>;

class CStringSelectorNode : public CSelectorNodeBase
{
protected:

	CStrID               _ParamID;
	UPTR                 _ParamIndex = INVALID_INDEX; // Cached for fast access

	CVariant             _TrueVariant;
	CVariant             _FalseVariant;

	virtual CVariant* SelectVariant(CAnimationUpdateContext& Context) override;

public:

	CStringSelectorNode(CStrID ParamID);

	virtual void Init(CAnimationInitContext& Context) override;
};

}
