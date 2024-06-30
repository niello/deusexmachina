#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/Ptr.h>

// Plays a Flow asset and tracks its state.
// Only one action can be active at the same time.

namespace DEM::Flow
{
using PFlowAsset = Ptr<class CFlowAsset>;
constexpr U32 EmptyActionID_FIXME = 0; //!!!!!!!!FIXME: fix duplicated definition!

struct CUpdateContext
{
	float       dt;
	U32         NextActionID;
	std::string Error;
	bool        Finished;
	bool        YieldToNextFrame;
};

class IFlowAction : public ::Core::CRTTIBaseClass
{
protected:

	// TODO: shortcuts for filling CUpdateContext for common cases: break, choose pin(pass dt consumed), throw error, continue updating

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
	void Finish();

public:

	~CFlowPlayer();

	bool Start(PFlowAsset Asset, U32 StartActionID = EmptyActionID_FIXME);
	void Stop();
	void Update(float dt); //???!!!if needs session, must be in DEMGame?!

	CFlowAsset* GetAsset() const { return _Asset.Get(); }
	// get curr action ID
};

}
