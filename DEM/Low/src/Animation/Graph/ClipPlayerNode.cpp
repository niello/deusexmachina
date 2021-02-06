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

void CClipPlayerNode::Update(CAnimationController& Controller, float dt, CSyncContext* pSyncContext)
{
	if (!_Sampler.GetClip()) return;

	const ESyncMethod SyncMethod = pSyncContext ? pSyncContext->Method : ESyncMethod::None;
	switch (SyncMethod)
	{
		case ESyncMethod::None:
		{
			_CurrClipTime = _Sampler.GetClip()->AdjustTime(_CurrClipTime + dt * _Speed, _Loop);

			::Sys::DbgOut("***CClipPlayerNode: master, time %lf (rel %lf), clip %s\n", _CurrClipTime,
				_CurrClipTime / _Sampler.GetClip()->GetDuration(), _ClipID.CStr());
			break;
		}
		case ESyncMethod::NormalizedTime:
		{
			_CurrClipTime = pSyncContext->NormalizedTime * _Sampler.GetClip()->GetDuration();

			::Sys::DbgOut("***CClipPlayerNode: time-synced, time %lf (rel %lf), clip %s\n", _CurrClipTime,
				_CurrClipTime / _Sampler.GetClip()->GetDuration(), _ClipID.CStr());
			break;
		}
		case ESyncMethod::PhaseMatching:
		{
			const float NormalizedTime = _Sampler.GetClip()->GetLocomotionPhaseNormalizedTime(pSyncContext->LocomotionPhase);
			if (NormalizedTime < 0.f)
			{
				// TODO: per-animation or per-node setting?
				constexpr bool FallbackToTimeSync = false;
				if (FallbackToTimeSync)
				{
					_CurrClipTime = pSyncContext->NormalizedTime * _Sampler.GetClip()->GetDuration();

					::Sys::DbgOut("***CClipPlayerNode: no phase, time-synced, time %lf (rel %lf), clip %s\n", _CurrClipTime,
						_CurrClipTime / _Sampler.GetClip()->GetDuration(), _ClipID.CStr());
				}
				else
				{
					_CurrClipTime = _Sampler.GetClip()->AdjustTime(_CurrClipTime + dt * _Speed, _Loop);

					::Sys::DbgOut("***CClipPlayerNode: no phase, not synced, time %lf (rel %lf), clip %s\n", _CurrClipTime,
						_CurrClipTime / _Sampler.GetClip()->GetDuration(), _ClipID.CStr());
				}
			}
			else
			{
				_CurrClipTime = NormalizedTime * _Sampler.GetClip()->GetDuration();

				::Sys::DbgOut("***CClipPlayerNode: phase-synced, time %lf (rel %lf), phase %lf, clip %s\n", _CurrClipTime,
					_CurrClipTime / _Sampler.GetClip()->GetDuration(), pSyncContext->LocomotionPhase, _ClipID.CStr());
			}
			break;
		}
		default: NOT_IMPLEMENTED; break;
	}
}
//---------------------------------------------------------------------

void CClipPlayerNode::EvaluatePose(IPoseOutput& Output)
{
	if (_PortMapping)
		_Sampler.EvaluatePose(_CurrClipTime, CMappedPoseOutput(Output, _PortMapping.get()));
	else
		_Sampler.EvaluatePose(_CurrClipTime, Output);
}
//---------------------------------------------------------------------

float CClipPlayerNode::GetAnimationLengthScaled() const
{
	return (_Sampler.GetClip() && _Speed) ? (_Sampler.GetClip()->GetDuration() / _Speed) : 0.f;
}
//---------------------------------------------------------------------

float CClipPlayerNode::GetLocomotionPhase() const
{
	return _Sampler.GetClip() ?
		_Sampler.GetClip()->GetLocomotionPhase(_CurrClipTime / _Sampler.GetClip()->GetDuration()) :
		std::numeric_limits<float>().lowest();
}
//---------------------------------------------------------------------

bool CClipPlayerNode::HasLocomotion() const
{
	return _Sampler.GetClip() && _Sampler.GetClip()->GetLocomotionInfo();
}
//---------------------------------------------------------------------

}
