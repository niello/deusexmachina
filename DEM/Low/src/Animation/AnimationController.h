#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <map>

// Animation controller plays an animation graph instance, feeding it with parameters.
// User specifies parameter values as an input and receives a pose as an output.

namespace Resources
{
	class CResourceManager;
}

namespace DEM::Anim
{
//using PAnimationController = std::unique_ptr<class CAnimationController>;
using PAnimGraphNode = std::unique_ptr<class CAnimGraphNode>;
using PSkeletonInfo = Ptr<class CSkeletonInfo>;
class IPoseOutput;

struct CAnimationControllerInitContext
{
	PSkeletonInfo&               SkeletonInfo;
	Resources::CResourceManager& ResourceManager;
	std::map<CStrID, CStrID>     AssetOverrides;
};

class CAnimationController
{
protected:

	PAnimGraphNode _GraphRoot;
	PSkeletonInfo  _SkeletonInfo;

	// parameters
	// shared conditions (allow nesting or not? if nested, must control cyclic dependencies and enforce calculation order)
	// NB: each condition, shared or not, must cache its value and recalculate only if used parameter values changed!

public:

	//???default node must skip Update and return reference pose from eval?

	void Init(Resources::CResourceManager& ResMgr, std::map<CStrID, CStrID> AssetOverrides);
	//bool BindOutput() - Prepare/Tune external output instead of binding and storing into the node?
	void Update(float dt);
	void EvaluatePose(IPoseOutput& Output);

	// SetBool
	// SetInt
	// SetFloat
	// SetStrID
	// Get...?

	const CSkeletonInfo* GetSkeletonInfo() const { return _SkeletonInfo.Get(); }
};

}
