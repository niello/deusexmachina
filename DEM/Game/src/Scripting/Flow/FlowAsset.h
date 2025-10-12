#pragma once
#include <Scripting/Flow/FlowCommon.h>
#include <Scripting/Condition.h>
#include <Core/Object.h>

// Reusable and stateless asset representing a node based Flow script.
// Flow consists of actions linked with links which may be conditionally disabled.

namespace DEM::Flow
{

struct CFlowLink
{
	Game::CConditionData Condition;
	U32                  DestID = EmptyActionID;
	bool                 YieldToNextFrame = false;
};

struct CFlowActionData
{
	CStrID                 ClassName; //TODO: use FourCC?! or register class names in a factory as CStrIDs?!
	Data::PParams          Params;
	std::vector<CFlowLink> Links;
	U32                    ID = EmptyActionID;
};

class CFlowAsset : public DEM::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Flow::CFlowAsset, DEM::Core::CObject);

protected:

	std::vector<CFlowActionData> _Actions;
	Game::CGameVarStorage        _VarStorage;
	U32                          _DefaultStartActionID;

public:

	CFlowAsset(std::vector<CFlowActionData>&& Actions, Game::CGameVarStorage&& Vars, U32 StartActionID)
		: _Actions(std::move(Actions))
		, _VarStorage(std::move(Vars))
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
		auto It = std::lower_bound(_Actions.cbegin(), _Actions.cend(), ID, [](const auto& Elm, U32 Value) { return Elm.ID < Value; });
		return (It == _Actions.cend() || (*It).ID != ID) ? nullptr : &(*It);
	}

	bool HasAction(CStrID ClassName) const
	{
		auto It = std::find_if(_Actions.cbegin(), _Actions.cend(), [ClassName](const auto& Elm) { return Elm.ClassName == ClassName; });
		return It != _Actions.cend();
	}

	U32 GetDefaultStartActionID() const { return _DefaultStartActionID; }
	auto& GetDefaultVarStorage() const { return _VarStorage; }
};

using PFlowAsset = Ptr<CFlowAsset>;

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::Flow::CFlowLink>() { return "DEM::Flow::CFlowLink"; }
template<> constexpr auto RegisterMembers<DEM::Flow::CFlowLink>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Flow::CFlowLink, Condition),
		DEM_META_MEMBER_FIELD(Flow::CFlowLink, DestID),
		DEM_META_MEMBER_FIELD(Flow::CFlowLink, YieldToNextFrame)
	);
}

template<> constexpr auto RegisterClassName<DEM::Flow::CFlowActionData>() { return "DEM::Flow::CFlowActionData"; }
template<> constexpr auto RegisterMembers<DEM::Flow::CFlowActionData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Flow::CFlowActionData, ClassName),
		DEM_META_MEMBER_FIELD(Flow::CFlowActionData, Params),
		DEM_META_MEMBER_FIELD(Flow::CFlowActionData, Links),
		DEM_META_MEMBER_FIELD(Flow::CFlowActionData, ID)
	);
}

}
