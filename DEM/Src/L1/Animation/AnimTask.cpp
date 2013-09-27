#include "AnimTask.h"

#include <Scene/SceneNode.h>
#include <Scene/NodeControllerComposite.h>
#include <Animation/KeyframeClip.h>
#include <Animation/MocapClip.h>
#include <Animation/NodeControllerKeyframe.h>
#include <Animation/NodeControllerMocap.h>

namespace Anim
{

void CAnimTask::Update(float FrameTime)
{
	if (State == Task_LastFrame)
	{
		Clear();
		return;
	}

	if (State == Task_Paused || State == Task_Invalid) return;

	float PrevTime = CurrTime;
	if (State == Task_Starting)
	{
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			Ctlrs[i]->Activate(true);
		State = Task_Running;
		PrevRealWeight = Weight;

		// Fire events at initial time point, because interval-based firing below always excludes StartTime
		Clip->FireEvents(CurrTime, Loop, pEventDisp, Params);
	}
	else if (State == Task_Running || State == Task_Stopping)
		CurrTime += FrameTime * Speed;

	// Stop non-looping clips automatically
	if (!Loop && State == Task_Running)
	{
		// Such form of a condition supports both fwd & back animation directions
		if (Speed * (CurrTime - StopTimeBase) > 0.f)
			State = Task_Stopping;
	}

	// Apply optional fadein / fadeout, check for the end
	float RealWeight = Weight;
	if (State == Task_Stopping)
	{
		float CurrFadeOutTime = CurrTime - StopTimeBase;
		if (Speed * (CurrFadeOutTime - FadeOutTime) >= 0.f)
		{
			if (FadeOutTime == 0.f)
			{
				State = Task_LastFrame;
				CurrTime = StopTimeBase + FadeOutTime;
			}
			else
			{
				Clear();
				return;
			}
		}
		else RealWeight *= (1.f - CurrFadeOutTime / FadeOutTime);
	}
	else
	{
		if (Speed * (CurrTime - FadeInTime) < 0.f)
			RealWeight *= (CurrTime - Offset) / (FadeInTime - Offset);
	}

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
		((Anim::CMocapClip*)Clip.Get())->GetSamplingParams(CurrTime, Loop, KeyIndex, IpolFactor);
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			((Anim::CNodeControllerMocap*)Ctlrs[i].GetUnsafe())->SetSamplingParams(KeyIndex, IpolFactor);
	}
	else if (Clip->IsA<Anim::CKeyframeClip>())
	{
		float Time = Clip->AdjustTime(CurrTime, Loop);
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			((Anim::CNodeControllerKeyframe*)Ctlrs[i].GetUnsafe())->SetTime(Time);
	}

	// Fire animation events
	// NB: pass unadjusted time
	Clip->FireEvents(PrevTime, CurrTime, Loop, pEventDisp, Params);
}
//---------------------------------------------------------------------

void CAnimTask::SetPause(bool Pause)
{
	if (State == Task_Stopping || State == Task_Invalid) return; //???what to do with Starting?
	if (Pause == (State == Task_Paused)) return;
	for (int i = 0; i < Ctlrs.GetCount(); ++i)
		Ctlrs[i]->Activate(!Pause);
	State = Pause ? Task_Paused : Task_Running;
}
//---------------------------------------------------------------------

void CAnimTask::Stop(float OverrideFadeOutTime)
{
	if (State == Task_Stopping || State == Task_Invalid) return; //???what to do with Starting?

	if (FadeOutTime <= 0.f && OverrideFadeOutTime > 0.f)
	{
		n_printf("Anim,Warning: Overriding animation fade out time from 0 to any other value is not allowed due to disabled blending\n");
		OverrideFadeOutTime = 0.f;
	}

	if (OverrideFadeOutTime < 0.f) OverrideFadeOutTime = FadeOutTime;
	else OverrideFadeOutTime *= Speed;

	if (!Loop)
	{
		float MaxPossibleFadeOut = ((Speed > 0.f) ? Clip->GetDuration() : 0.f) - CurrTime;
		if (Speed * (MaxPossibleFadeOut - OverrideFadeOutTime) < 0.f)
			OverrideFadeOutTime = MaxPossibleFadeOut;
	}

	if (OverrideFadeOutTime == 0.f) Clear();
	else
	{
		State = Task_Stopping;
		StopTimeBase = CurrTime;
		FadeOutTime = OverrideFadeOutTime;
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