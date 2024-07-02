#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/Ptr.h>

// Plays a Flow asset and tracks its state.
// Only one action can be active at the same time.

namespace DEM::Flow
{
struct CFlowLink;
using PFlowAsset = Ptr<class CFlowAsset>;
constexpr U32 EmptyActionID_FIXME = 0; //!!!!!!!!FIXME: fix duplicated definition!

struct CUpdateContext
{
	float       dt = 0.f;
	U32         NextActionID = EmptyActionID_FIXME;
	std::string Error;
	bool        Finished = false;
	bool        YieldToNextFrame = false;
};

class IFlowAction : public ::Core::CRTTIBaseClass
{
protected:

	// Shortcuts for action flow control. Can be used with 'return' for shortness.
	static void Continue(CUpdateContext& Ctx);
	static void Break(CUpdateContext& Ctx);
	static void Throw(CUpdateContext& Ctx, std::string&& Error, bool CanRetry);
	static void Goto(CUpdateContext& Ctx, const CFlowLink& Link, float consumedDt = 0.f);

public:

	virtual void OnStart() = 0;
	virtual void Update(CUpdateContext& Ctx) = 0;
	virtual void OnCancel() {}
};

using PFlowAction = std::unique_ptr<IFlowAction>;

class CFlowPlayer
{
private:

	PFlowAsset  _Asset;
	PFlowAction _CurrAction;
	U32         _NextActionID = EmptyActionID_FIXME; // Non-empty when a link is yielding to the next frame
	// Variable set
	// TODO: add action pool Type->Instance? std::map<CStrID, PFlowAction>; // NB: always needs only 1 instance of each type.

	void SetCurrentAction(U32 ID);
	void Finish(bool WithError);

public:

	~CFlowPlayer();

	bool Start(PFlowAsset Asset, U32 StartActionID = EmptyActionID_FIXME);
	void Stop();
	void Update(float dt); //???!!!if needs session, must be in DEMGame?!

	CFlowAsset* GetAsset() const { return _Asset.Get(); }
	bool IsPlaying() const { return _CurrAction || _NextActionID != EmptyActionID_FIXME; }
};

}
