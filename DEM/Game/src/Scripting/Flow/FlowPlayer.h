#pragma once
#include <Core/RTTIBaseClass.h>
#include <Events/Signal.h>
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/VarStorage.h>

// Plays a Flow asset and tracks its state.
// Only one action can be active at the same time.

namespace DEM::Game
{
	using PGameSession = Ptr<class CGameSession>;
}

namespace DEM::Flow
{
struct CConditionData;
struct CFlowLink;
struct CFlowActionData;
class CFlowPlayer;
using PFlowAsset = Ptr<class CFlowAsset>;
using CFlowVarStorage = CVarStorage<bool, int, float, std::string, CStrID>;
constexpr U32 EmptyActionID_FIXME = 0; //!!!!!!!!FIXME: fix duplicated definition, see EmptyActionID!

//!!!TODO: can be universal, not flow-specific!
bool EvaluateCondition(const CConditionData& Cond, const Game::CGameSession& Session, const CFlowVarStorage& Vars);

struct CUpdateContext
{
	Game::CGameSession* pSession = nullptr;
	float               dt = 0.f;
	U32                 NextActionID = EmptyActionID_FIXME;
	std::string         Error;
	bool                Finished = false;
	bool                YieldToNextFrame = false;
};

class IFlowAction : public ::Core::CRTTIBaseClass
{
protected:

	// Shortcuts for action flow control. Can be used with 'return' for shortness.
	static void Continue(CUpdateContext& Ctx);
	static void Break(CUpdateContext& Ctx);
	static void Throw(CUpdateContext& Ctx, std::string&& Error, bool CanRetry);
	static void Goto(CUpdateContext& Ctx, const CFlowLink* pLink, float consumedDt = 0.f);

	template<typename F>
	static void ForEachValidLink(const CFlowActionData& Proto, const Game::CGameSession& Session, const CFlowVarStorage& Vars, F Callback)
	{
		const auto LinkCount = Proto.Links.size();
		for (size_t i = 0; i < LinkCount; ++i)
		{
			const auto& Link = Proto.Links[i];
			if (EvaluateCondition(Link.Condition, Session, Vars))
			{
				if constexpr (std::is_invocable_r_v<bool, F, size_t, const CFlowLink&>)
				{
					if (!Callback(i, Link)) return;
				}
				else if constexpr (std::is_invocable_v<F, size_t, const CFlowLink&>)
				{
					Callback(i, Link);
				}
				else static_assert(false, "Callback must accept link index and const link reference");
			}
		}
	}

	template<typename F>
	DEM_FORCE_INLINE void ForEachValidLink(const Game::CGameSession& Session, const CFlowVarStorage& Vars, F Callback) const
	{
		if (_pPrototype) ForEachValidLink<F>(*_pPrototype, Session, Vars, Callback);
	}

	const CFlowLink* GetFirstValidLink(const Game::CGameSession& Session, const CFlowVarStorage& Vars) const;

public:

	const CFlowActionData* _pPrototype = nullptr; // Can't set in a constructor because the main factory doesn't support constructor args
	CFlowPlayer*           _pPlayer = nullptr; // Can't set in a constructor because the main factory doesn't support constructor args

	virtual void OnStart(Game::CGameSession& Session) {}
	virtual void Update(CUpdateContext& Ctx) = 0;
	virtual void OnCancel() {}
};

using PFlowAction = std::unique_ptr<IFlowAction>;

class CFlowPlayer
{
private:

	PFlowAsset      _Asset;
	PFlowAction     _CurrAction;
	U32             _NextActionID = EmptyActionID_FIXME; // Non-empty when a link is yielding to the next frame
	CFlowVarStorage _VarStorage;
	// TODO: add action pool Type->Instance? std::map<CStrID, PFlowAction>; // NB: always needs only 1 instance of each type.

	void SetCurrentAction(Game::CGameSession& Session, U32 ID);
	void Finish(bool WithError);

public:

	DEM::Events::CSignal<void()>                                         OnStart;
	DEM::Events::CSignal<void(U32 /*LastActionID*/, bool /*WithError*/)> OnFinish;

	~CFlowPlayer();

	bool Start(PFlowAsset Asset, U32 StartActionID = EmptyActionID_FIXME);
	void Stop();
	void Update(Game::CGameSession& Session, float dt);

	CFlowAsset* GetAsset() const { return _Asset.Get(); }
	auto&       GetVars() { return _VarStorage; }
	auto&       GetVars() const { return _VarStorage; }
	bool        IsPlaying() const { return _CurrAction || _NextActionID != EmptyActionID_FIXME; }
};

}