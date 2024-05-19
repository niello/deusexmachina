#include "ClipPlayerNode.h"
#include <Animation/AnimationController.h>
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Animation/MappedPoseOutput.h>
#include <Animation/Timeline/EventClip.h>
#include <Events/EventOutput.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>

namespace DEM::Anim
{

CClipPlayerNode::CClipPlayerNode(CStrID ClipID, bool Loop, float Speed, float StartTime, bool ResetOnActivate)
	: _ClipID(ClipID)
	, _StartTime(StartTime)
	, _Speed(Speed)
	, _Loop(Loop)
	, _ResetOnActivate(ResetOnActivate)
{
}
//---------------------------------------------------------------------

CClipPlayerNode::~CClipPlayerNode() = default;
//---------------------------------------------------------------------

void CClipPlayerNode::ResetTime()
{
	if (_Sampler.GetClip() && _Speed < 0.f && _StartTime == 0.f)
		_CurrClipTime = _Sampler.GetClip()->GetDuration();
	else
		_CurrClipTime = _StartTime;
}
//---------------------------------------------------------------------

// Returns wrapped time and wrap count
std::pair<float, U32> CClipPlayerNode::GetWrappedTime(float dt) const
{
	const float NewTime = _CurrClipTime + dt * _Speed;
	const float Duration = _Sampler.GetClip()->GetDuration();

	if (!_Loop)
		return { std::clamp(NewTime, 0.f, Duration), 0 };

	float WrapCount;
	const float NewTimeWrapped = std::modf(NewTime / Duration, &WrapCount) * Duration;
	if (std::signbit(NewTimeWrapped))
		return { Duration + NewTimeWrapped, static_cast<U32>(-WrapCount) };
	else
		return { NewTimeWrapped, static_cast<U32>(WrapCount) };
}
//---------------------------------------------------------------------

void CClipPlayerNode::Init(CAnimationInitContext& Context)
{
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

	ResetTime(); // NB: must be after initializing the clip
	_LastUpdateIndex = Context.Controller.GetUpdateIndex();
}
//---------------------------------------------------------------------

void CClipPlayerNode::Update(CAnimationUpdateContext& Context, float dt)
{
	const auto pClip = _Sampler.GetClip();
	if (!pClip || pClip->GetDuration() <= 0.f || _Speed == 0.f) return;

	const U32 CurrUpdateIndex = Context.Controller.GetUpdateIndex();
	const bool WasInactive = (_LastUpdateIndex != CurrUpdateIndex - 1) || !IsActive(); // Was not updated on prev frame or played to the end without looping
	if (_ResetOnActivate && WasInactive) ResetTime();
	_LastUpdateIndex = CurrUpdateIndex;

	const float PrevClipTime = _CurrClipTime;
	U32 WrapCount = 0;

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
			WrapCount = Context.LocomotionWrapCount;

			//::Sys::DbgOut("***CClipPlayerNode: phase-synced, time %lf (rel %lf), phase %lf, clip %s\n", _CurrClipTime,
			//	_CurrClipTime / pClip->GetDuration(), Context.LocomotionPhase, _ClipID.CStr());
		}
		else
		{
			// We drive the phase by our normal time update, others sync with us
			std::tie(_CurrClipTime, WrapCount) = GetWrappedTime(dt);
			Context.LocomotionPhase = pClip->GetLocomotionPhase(_CurrClipTime / pClip->GetDuration());
			Context.LocomotionWrapCount = WrapCount;

			//::Sys::DbgOut("***CClipPlayerNode: phase-driving, time %lf (rel %lf), phase %lf, clip %s\n", _CurrClipTime,
			//	_CurrClipTime / pClip->GetDuration(), Context.LocomotionPhase, _ClipID.CStr());
		}
	}
	else
	{
		// TODO: add normalized time syncing option? Not used for now.

		// Update regular clip
		std::tie(_CurrClipTime, WrapCount) = GetWrappedTime(dt);

		//::Sys::DbgOut("***CClipPlayerNode: no sync, time %lf (rel %lf), clip %s\n", _CurrClipTime,
		//	_CurrClipTime / pClip->GetDuration(), _ClipID.CStr());
	}

	if (Context.pEventOutput)
	{
		float BeginTime = 0.f;
		float EndTime = pClip->GetDuration();
		if (_Speed < 0.f) std::swap(BeginTime, EndTime);

		if (auto pEventClip = pClip->GetEventClip())
		{
			float FromTime = PrevClipTime;
			bool IncludeStartTime = WasInactive;
			for (U32 i = 0; i < WrapCount; ++i)
			{
				pEventClip->PlayInterval(FromTime, EndTime, *Context.pEventOutput, IncludeStartTime);
				FromTime = BeginTime;
				IncludeStartTime = true;
			}

			pEventClip->PlayInterval(FromTime, _CurrClipTime, *Context.pEventOutput, IncludeStartTime);
		}

		// Fire automatic end event for play-once animations
		if (!_Loop)
		{
			if (PrevClipTime != EndTime && _CurrClipTime == EndTime)
				Context.pEventOutput->OnEvent(Event_AnimEnd, {}, PrevClipTime + dt * _Speed - _CurrClipTime);
		}
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
	if (!_Sampler.GetClip()) return false;
	if (_Loop) return true;
	return (_Speed > 0.f) ? (_CurrClipTime < _Sampler.GetClip()->GetDuration()) : (_CurrClipTime > 0.f);
}
//---------------------------------------------------------------------

}
