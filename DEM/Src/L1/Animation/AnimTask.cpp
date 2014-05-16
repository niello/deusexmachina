#include "AnimTask.h"

#include <Scene/SceneNode.h>
#include <Scene/NodeControllerComposite.h>
#include <Animation/KeyframeClip.h>
#include <Animation/MocapClip.h>
#include <Animation/NodeControllerKeyframe.h>
#include <Animation/NodeControllerMocap.h>

namespace Anim
{

void CAnimTask::Init(Anim::PAnimClip _Clip, bool _Loop, float _Offset, float _Speed, float _Weight, float FadeInTime, float FadeOutTime)
{
	n_assert(_Clip.IsValid() && _Speed != 0.f);

	Clip = _Clip;
	Loop = _Loop;
	Offset = _Offset;
	Speed = _Speed;
	Weight = _Weight;

	CursorPos = Offset;
	NewCursorPos = CursorPos;

	// Convert from real time to animation timeline scale
	float FadeInLength = FadeInTime * Speed;
	FadeOutLength = FadeOutTime * Speed;

	if (!Loop)
	{
		if (Speed > 0.f)
		{
			if (Offset == Clip->GetDuration())
			{
				Sys::Log("Anim,Warning: Forward non-looping animation with offset = Duration will not be played, because it is its end. Fixed to 0.");
				CursorPos = 0.f;
			}
			if (FadeInLength + FadeOutLength > Clip->GetDuration())
			{
				FadeOutLength = n_max(0.f, Clip->GetDuration() - FadeInLength);
				FadeInLength = Clip->GetDuration() - FadeOutLength;
			}
			FadeOutStartPos = Clip->GetDuration() - FadeOutLength;
		}
		else
		{
			if (Offset == 0.f)
			{
				Sys::Log("Anim,Warning: Reverse non-looping animation with offset = 0 will not be played, because it is its end. Fixed to Duration.");
				CursorPos = Clip->GetDuration();
			}
			if (FadeInLength + FadeOutLength < -Clip->GetDuration())
			{
				FadeOutLength = n_min(0.f, -Clip->GetDuration() - FadeInLength);
				FadeInLength = -Clip->GetDuration() - FadeOutLength;
			}
			FadeOutStartPos = -FadeOutLength;
		}
	}

	FadeInEndPos = Offset + FadeInLength * Speed;

	State = Anim::CAnimTask::Task_Starting;
}
//---------------------------------------------------------------------

void CAnimTask::Update()
{
	if (State == Task_LastFrame) Clear();
	if (State == Task_Invalid) return;

	float PrevCursorPos = CursorPos;
	if (State == Task_Starting)
	{
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			Ctlrs[i]->Activate(true);
		State = Loop ? Task_Looping : Task_Running;
		PrevRealWeight = 1.f; // Ensure the weight will be applied if it is not 1.f

		// Fire events at initial time point, because interval-based firing below always excludes StartTime
		Clip->FireEvents(CursorPos, Loop, pEventDisp, Params);
	}
	else // Running or Looping
	{
		if (NewCursorPos == CursorPos) return;
		CursorPos = NewCursorPos;
	}

	// Apply optional fadein / fadeout, check for the end
	float RealWeight = Weight;
	if (State != Task_Looping && Speed * (CursorPos - FadeOutStartPos) > 0.f)
	{
		float CurrFadeOutLength = CursorPos - FadeOutStartPos;
		if (Speed * (CurrFadeOutLength - FadeOutLength) >= 0.f)
		{
			if (FadeOutLength == 0.f)
			{
				State = Task_LastFrame;
				CursorPos = FadeOutStartPos + FadeOutLength;
			}
			else
			{
				Clear();
				return;
			}
		}
		else RealWeight *= (1.f - CurrFadeOutLength / FadeOutLength);
	}
	else if (Speed * (CursorPos - FadeInEndPos) < 0.f)
		RealWeight *= (CursorPos - Offset) / (FadeInEndPos - Offset);

	// Apply weight, if it has changed
	if (PrevRealWeight != RealWeight)
	{
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
		{
			Scene::CNodeController* pCtlr = Ctlrs[i];
			if (!pCtlr->IsAttachedToNode()) continue;
			n_assert_dbg(pCtlr->GetNode()->GetController()->IsA<Scene::CNodeControllerComposite>());
			Scene::CNodeControllerComposite* pBlendCtlr = (Scene::CNodeControllerComposite*)pCtlr->GetNode()->GetController();
			int Idx = ((Scene::CNodeControllerComposite*)pBlendCtlr)->GetSourceIndex(*pCtlr);
			n_assert_dbg(Idx != INVALID_INDEX);
			pBlendCtlr->SetWeight(Idx, RealWeight); //???mb store task ptr in controller?
		}
		PrevRealWeight = RealWeight;
	}

	// Feed node controllers
	if (Clip->IsA<Anim::CMocapClip>())
	{
		int KeyIndex;
		float IpolFactor;
		((Anim::CMocapClip*)Clip.Get())->GetSamplingParams(CursorPos, Loop, KeyIndex, IpolFactor);
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			((Anim::CNodeControllerMocap*)Ctlrs[i].GetUnsafe())->SetSamplingParams(KeyIndex, IpolFactor);
	}
	else if (Clip->IsA<Anim::CKeyframeClip>())
	{
		float Time = Clip->AdjustCursorPos(CursorPos, Loop);
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			((Anim::CNodeControllerKeyframe*)Ctlrs[i].GetUnsafe())->SetTime(Time);
	}

	// Fire animation events
	// NB: pass unadjusted time
	Clip->FireEvents(PrevCursorPos, CursorPos, Loop, pEventDisp, Params);
}
//---------------------------------------------------------------------

void CAnimTask::Stop(float OverrideFadeOutTime)
{
	if (State == Task_Starting || State == Task_Invalid)
	{
		Sys::Log("Anim,Warning: Can't stop starting or invalid animation task\n");
		return;
	}

	if (FadeOutLength <= 0.f && OverrideFadeOutTime > 0.f)
		Sys::Log("Anim,Warning: Overriding animation fade out time from 0 to any other value is not allowed due to disabled blending\n");
	else if (OverrideFadeOutTime >= 0.f) FadeOutLength = OverrideFadeOutTime * Speed;

	if (!Loop) 
	{
		float MaxPossibleFadeOut = ((Speed > 0.f) ? Clip->GetDuration() : 0.f) - CursorPos;
		if (Speed * (MaxPossibleFadeOut - FadeOutLength) < 0.f)
			FadeOutLength = MaxPossibleFadeOut;
	}

	if (FadeOutLength == 0.f)
	{
		//Clear();
		State = Task_LastFrame;
	}
	else
	{
		if (Loop) State = Task_Running; // Stop looping, enable fade-out
		FadeOutStartPos = CursorPos;
	}
}
//---------------------------------------------------------------------

void CAnimTask::Clear()
{
	State = Task_Invalid;
	pEventDisp = NULL;
	Params = NULL;
	Clip = NULL;
	for (int i = 0; i < Ctlrs.GetCount(); ++i)
	{
		Ctlrs[i]->RemoveFromNode();
		n_assert_dbg(Ctlrs[i]->GetRefCount() == 1); // Could be stored in detached blend controller, if it is stored somewhere
	}
	Ctlrs.Clear();
}
//---------------------------------------------------------------------

}