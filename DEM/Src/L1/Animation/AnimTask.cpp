#include "AnimTask.h"

#include <Scene/SceneNode.h>
#include <Animation/KeyframeClip.h>
#include <Animation/MocapClip.h>
#include <Animation/NodeControllerKeyframe.h>
#include <Animation/NodeControllerMocap.h>

namespace Anim
{

void CAnimTask::Update(float FrameTime)
{
	if (State == Task_Paused || State == Task_Invalid) return;

	if (State == Task_Starting)
	{
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			Ctlrs.ValueAt(i)->Activate(true);
		State = Task_Running;
	}
	else if (State == Task_Running)
		CurrTime += FrameTime * Speed;

	// Stop non-looping clips automatically
	if (!Loop && State == Task_Running)
	{
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
			Clear();
			return;
		}

		RealWeight *= CurrFadeOutTime / FadeOutTime;
	}
	else
	{
		if (Speed * (CurrTime - FadeInTime) < 0.f)
			RealWeight *= (CurrTime - Offset) / (FadeInTime - Offset);
	}

	// Feed node controllers
	if (Clip->IsA<Anim::CMocapClip>())
	{
		int KeyIndex;
		float IpolFactor;
		((Anim::CMocapClip*)Clip.Get())->GetSamplingParams(CurrTime, Loop, KeyIndex, IpolFactor);
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			((Anim::CNodeControllerMocap*)Ctlrs.ValueAt(i))->SetSamplingParams(KeyIndex, IpolFactor);
	}
	else if (Clip->IsA<Anim::CKeyframeClip>())
	{
		float Time = Clip->AdjustTime(CurrTime, Loop);
		for (int i = 0; i < Ctlrs.GetCount(); ++i)
			((Anim::CNodeControllerKeyframe*)Ctlrs.ValueAt(i))->SetTime(Time);
	}
}
//---------------------------------------------------------------------

void CAnimTask::SetPause(bool Pause)
{
	if (State == Task_Stopping || State == Task_Invalid) return; //???what to do with Starting?
	if (Pause == (State == Task_Paused)) return;
	for (int i = 0; i < Ctlrs.GetCount(); ++i)
		Ctlrs.ValueAt(i)->Activate(!Pause);
	State = Pause ? Task_Paused : Task_Running;
}
//---------------------------------------------------------------------

void CAnimTask::Stop(float OverrideFadeOutTime)
{
	if (State == Task_Stopping || State == Task_Invalid) return; //???what to do with Starting?

	if (OverrideFadeOutTime < 0.f) OverrideFadeOutTime = FadeOutTime;
	else OverrideFadeOutTime *= Speed;

	if (!Loop)
	{
		float MaxPossibleFadeOut = ((Speed > 0.f) ? Clip->GetDuration() : 0.f) - CurrTime;
		if (Speed * (MaxPossibleFadeOut - OverrideFadeOutTime) < 0.f)
			OverrideFadeOutTime = MaxPossibleFadeOut;
	}

	if (OverrideFadeOutTime <= 0.f) Clear();
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
	ClipID = CStrID::Empty;
	Clip = NULL;
	for (int i = 0; i < Ctlrs.GetCount(); ++i)
	{
		n_assert_dbg(Ctlrs.KeyAt(i)->Controller.GetUnsafe() == Ctlrs.ValueAt(i));
		Ctlrs.KeyAt(i)->Controller = NULL;
	}
	Ctlrs.Clear();
}
//---------------------------------------------------------------------

}