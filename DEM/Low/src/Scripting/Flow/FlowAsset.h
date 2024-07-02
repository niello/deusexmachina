#pragma once
#include <Core/Object.h>
#include <Data/Params.h>
#include <Data/Metadata.h>
#include <map>

// Reusable and stateless asset representing a node based Flow script.
// Flow consists of actions linked with links which may be conditionally disabled.

namespace DEM::Flow
{
constexpr U32 EmptyActionID = 0;

//!!!can be universal, not flow-specific!
struct CConditionData
{
	CStrID        Type;
	Data::PParams Params;
};

struct CFlowLink
{
	CConditionData Condition; // Empty type means no condition
	U32            DestID;
	bool           YieldToNextFrame;
};

struct CFlowActionData
{
	U32                    ID;
	CStrID                 ClassName; //TODO: use FourCC?! or register class names in a factory as CStrIDs?!
	Data::PParams          Params;
	std::vector<CFlowLink> Links;
};

class CFlowAsset : public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Flow::CFlowAsset, ::Core::CObject);

protected:

	std::vector<CFlowActionData> _Actions;
	U32                          _DefaultStartActionID;

	// TODO: variable storage with default values

	//!!!values should support HEntity, but it is in DEMGame, maybe can use std::any? or some extensible type system? or move Flow to DEMGame?
	//!!!need variable storage default value deserialization for HEntity. Deserialization from HRD already exists, need to use properly.

public:

	CFlowAsset(std::vector<CFlowActionData>&& Actions, U32 StartActionID)
		: _Actions(std::move(Actions))
		, _DefaultStartActionID(StartActionID)
	{
		std::sort(_Actions.begin(), _Actions.end(), [](const auto& a, const auto& b) { return a.ID < b.ID; });

#if _DEBUG
		// Check that all IDs are unique and not empty
		for (size_t i = 0; i < _Actions.size(); ++i)
		{
			n_assert(_Actions[i].ID != DEM::Flow::EmptyActionID);
			n_assert(i == 0 || _Actions[i - 1].ID != _Actions[i].ID);
		}
#endif
	}

	const CFlowActionData* FindAction(U32 ID) const
	{
		auto It = std::lower_bound(_Actions.begin(), _Actions.end(), ID, [](const auto& Elm, U32 Value) { return Elm.ID < Value; });
		return (It == _Actions.end() || (*It).ID != ID) ? nullptr : &(*It);
	}

	U32 GetDefaultStartActionID() const { return _DefaultStartActionID; }
};

using PFlowAsset = Ptr<CFlowAsset>;

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::Flow::CConditionData>() { return "DEM::Flow::CConditionData"; }
template<> inline constexpr auto RegisterMembers<DEM::Flow::CConditionData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Flow::CConditionData, 1, Type),
		DEM_META_MEMBER_FIELD(Flow::CConditionData, 2, Params)
	);
}

template<> inline constexpr auto RegisterClassName<DEM::Flow::CFlowLink>() { return "DEM::Flow::CFlowLink"; }
template<> inline constexpr auto RegisterMembers<DEM::Flow::CFlowLink>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Flow::CFlowLink, 1, Condition),
		DEM_META_MEMBER_FIELD(Flow::CFlowLink, 2, DestID),
		DEM_META_MEMBER_FIELD(Flow::CFlowLink, 3, YieldToNextFrame)
	);
}

template<> inline constexpr auto RegisterClassName<DEM::Flow::CFlowActionData>() { return "DEM::Flow::CFlowActionData"; }
template<> inline constexpr auto RegisterMembers<DEM::Flow::CFlowActionData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Flow::CFlowActionData, 1, ID),
		DEM_META_MEMBER_FIELD(Flow::CFlowActionData, 2, ClassName),
		DEM_META_MEMBER_FIELD(Flow::CFlowActionData, 3, Params),
		DEM_META_MEMBER_FIELD(Flow::CFlowActionData, 4, Links)
	);
}

}
