#include "ClipPlayerNode.h"
#include <Animation/AnimationController.h>
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Animation/MappedPoseOutput.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>

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

void CClipPlayerNode::Init(CAnimationInitContext& Context)
{
	_CurrClipTime = _StartTime;
	_LastUpdateIndex = Context.Controller.GetUpdateIndex();

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

void CClipPlayerNode::Update(CAnimationUpdateContext& Context, float dt)
{
	const auto pClip = _Sampler.GetClip();
	if (!pClip || pClip->GetDuration() <= 0.f) return;

	const U32 CurrUpdateIndex = Context.Controller.GetUpdateIndex();
	const bool WasInactive = (_LastUpdateIndex != CurrUpdateIndex - 1);
	if (_ResetOnActivate && WasInactive) _CurrClipTime = _StartTime;
	_LastUpdateIndex = CurrUpdateIndex;

	if (pClip->GetLocomotionInfo())
	{
		if (Context.LocomotionPhase < 0.f && WasInactive)
		{
			// We just started locomotion, sync the phase from the current pose if possible
			Context.LocomotionPhase = Context.Controller.GetLocomotionPhaseFromPose(Context.Target);

			constexpr float LOCOMOTION_BLEND_IN_TIME = 0.5f;
			if (Context.LocomotionPhase >= 0.f)
				Context.Controller.RequestInertialization(LOCOMOTION_BLEND_IN_TIME);

			//::Sys::DbgOut("***CClipPlayerNode: phase from pose %lf\n", Context.LocomotionPhase);
		}

		if (Context.LocomotionPhase >= 0.f)
		{
			// Locomotion phase is already evaluated, sync clip with it
			// NB: for now the first locomotion clip determines phase, not the most weighted one. May change later.
			_CurrClipTime = pClip->GetLocomotionPhaseNormalizedTime(Context.LocomotionPhase) * pClip->GetDuration();

			//::Sys::DbgOut("***CClipPlayerNode: phase-synced, time %lf (rel %lf), phase %lf, clip %s\n", _CurrClipTime,
			//	_CurrClipTime / pClip->GetDuration(), Context.LocomotionPhase, _ClipID.CStr());
		}
		else
		{
			// We drive the phase by our normal time update, others sync with us
			_CurrClipTime = pClip->AdjustTime(_CurrClipTime + dt * _Speed, _Loop);
			Context.LocomotionPhase = pClip->GetLocomotionPhase(_CurrClipTime / pClip->GetDuration());

			//::Sys::DbgOut("***CClipPlayerNode: phase-driving, time %lf (rel %lf), phase %lf, clip %s\n", _CurrClipTime,
			//	_CurrClipTime / pClip->GetDuration(), Context.LocomotionPhase, _ClipID.CStr());
		}
	}
	else
	{
		// TODO: add normalized time syncing option? Not used for now.

		// Update regular clip
		_CurrClipTime = pClip->AdjustTime(_CurrClipTime + dt * _Speed, _Loop);

		//::Sys::DbgOut("***CClipPlayerNode: no sync, time %lf (rel %lf), clip %s\n", _CurrClipTime,
		//	_CurrClipTime / pClip->GetDuration(), _ClipID.CStr());
	}
}
//---------------------------------------------------------------------

void CClipPlayerNode::EvaluatePose(CPoseBuffer& Output)
{
	_Sampler.EvaluatePose(_CurrClipTime, Output, _PortMapping.get());
}
//---------------------------------------------------------------------

float CClipPlayerNode::GetAnimationLengthScaled() const
{
	return (_Sampler.GetClip() && _Speed) ? (_Sampler.GetClip()->GetDuration() / _Speed) : 0.f;
}
//---------------------------------------------------------------------

bool CClipPlayerNode::IsActive() const
{
	return !_Sampler.GetClip() || (!_Loop && _CurrClipTime >= _Sampler.GetClip()->GetDuration());
}
//---------------------------------------------------------------------

}
