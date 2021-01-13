#include "ClipPlayerNode.h"
#include <Animation/AnimationController.h>
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>

//!!!DBG TMP!
#include <Animation/MappedPoseOutput.h>

namespace DEM::Anim
{

CClipPlayerNode::CClipPlayerNode(CStrID ClipID, bool Loop, float Speed, float StartTime)
	: _ClipID(ClipID)
	, _StartTime(StartTime)
	, _Speed(Speed)
	, _Loop(Loop)
{
}
//---------------------------------------------------------------------

void CClipPlayerNode::Init(CAnimationControllerInitContext& Context)
{
	_CurrClipTime = _StartTime;

	CStrID ClipID = _ClipID;
	if (!Context.AssetOverrides.empty())
	{
		auto It = Context.AssetOverrides.find(ClipID);
		if (It != Context.AssetOverrides.cend())
			ClipID = It->second;
	}

	auto AnimRsrc = Context.ResourceManager.RegisterResource<CAnimationClip>(ClipID.CStr());
	if (auto Anim = AnimRsrc->ValidateObject<CAnimationClip>())
	{
		CSkeletonInfo::Combine(Context.SkeletonInfo, Anim->GetSkeletonInfo(), _PortMapping);
		_Sampler.SetClip(Anim);
	}
}
//---------------------------------------------------------------------

void CClipPlayerNode::Update(CAnimationController& Controller, float dt)
{
	if (_Sampler.GetClip())
		_CurrClipTime = _Sampler.GetClip()->AdjustTime(_CurrClipTime + dt * _Speed, _Loop);

	//???where to handle synchronization? pass sync info into a context and sync after graph update?
}
//---------------------------------------------------------------------

void CClipPlayerNode::EvaluatePose(IPoseOutput& Output)
{
	if (_PortMapping)
	{
		CStackMappedPoseOutput MappedOutput(Output, _PortMapping.get());
		_Sampler.Apply(_CurrClipTime, MappedOutput);
	}
	else
	{
		_Sampler.Apply(_CurrClipTime, Output);
	}
}
//---------------------------------------------------------------------

}
