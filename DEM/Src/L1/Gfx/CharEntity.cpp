#include "CharEntity.h"

#include <Gfx/GfxServer.h>
#include <variable/nvariableserver.h>
#include <character/ncharacter2.h>
#include <scene/nskinanimator.h>

extern const matrix44 Rotate180;

namespace Graphics
{
ImplementRTTI(Graphics::CCharEntity, Graphics::CShapeEntity);
ImplementFactory(Graphics::CCharEntity);

CCharEntity::CCharEntity():
	BaseAnimStarted(0.0),
	BaseAnimOffset(0.0),
	BaseAnimDuration(0.0),
	BaseAnimFadeIn(0.0),
	OverlayAnimStarted(0.0),
	OverlayAnimDuration(0.0),
	RestartOverlayAnim(0),
	OverlayEndFadeIn(0.0),
	pNCharacter(NULL),
	pCharacterSet(NULL),
	pAnimEventHandler(NULL),
	ActivateNewBaseAnim(false)
{
	HCharacter = nVariableServer::Instance()->GetVariableHandleByName("charPointer");
	HCharacterSet = nVariableServer::Instance()->GetVariableHandleByName("charSetPointer");
	HChnRestartAnim = nVariableServer::Instance()->GetVariableHandleByName("chnRestartAnim");
	HAnimTimeOffset = nVariableServer::Instance()->GetVariableHandleByName("Offset");
	RenderCtx.AddVariable(nVariable(HAnimTimeOffset, 0.0f));
	RenderCtx.AddVariable(nVariable(HChnRestartAnim, 0));
}
//---------------------------------------------------------------------

void CCharEntity::Activate()
{
	n_assert(!pNCharacter);
	n_assert(!pAnimEventHandler);

	CShapeEntity::Activate();

	pAnimEventHandler = n_new(CCharAnimEHandler);
	n_assert(pAnimEventHandler);
	pAnimEventHandler->Entity = this;

	// lookup character pointer
	nVariable* pVar = RenderCtx.FindLocalVar(HCharacter);
	if (pVar)
	{
		pNCharacter = (nCharacter2*)pVar->GetObj();
		n_assert(pNCharacter);
		pNCharacter->SetAnimEventHandler(pAnimEventHandler);
	}

	// lookup character set
	HCharacterSet = pNCharacter->GetSkinAnimator()->GetCharacterSetIndexHandle();
	const nVariable& CharacterSetVar = RenderCtx.GetLocalVar(HCharacterSet);
	pCharacterSet = (nCharacter2Set*)CharacterSetVar.GetObj();
}
//---------------------------------------------------------------------

void CCharEntity::Deactivate()
{
	n_assert(pAnimEventHandler);

	if (pNCharacter)
	{
		pNCharacter->SetAnimEventHandler(NULL);
		pNCharacter = NULL;
	}

	n_assert(pAnimEventHandler);// && pAnimEventHandler->GetRefCount() == 1);
	pAnimEventHandler->Entity = NULL;
	pAnimEventHandler->Release();
	pAnimEventHandler = NULL;

	CShapeEntity::Deactivate();
}
//---------------------------------------------------------------------

int CCharEntity::GetNumAnimations() const
{
	if (pNCharacter)
	{
		n_assert(pNCharacter->GetSkinAnimator());
		return pNCharacter->GetSkinAnimator()->GetNumClips();
	}
	return 0;
}
//---------------------------------------------------------------------

// Set a new base animation. This is usually a looping animation, like Idle, Walking, Running, etc...
void CCharEntity::SetBaseAnimation(const nString& AnimName, nTime FadeIn, nTime Offset, bool OnlyIfInactive)
{
	n_assert(AnimName.IsValid());

	if (OnlyIfInactive && !BaseAnimNames.Empty() && BaseAnimNames[0] == AnimName)  return;

	//???can avoid?
	nArray<nString> Anims;
	Anims.Append(AnimName);
	nArray<float> Weights;
	Weights.Append(1.0f);

	SetBaseAnimationMix(Anims, Weights, FadeIn, Offset);
}
//---------------------------------------------------------------------

void CCharEntity::SetBaseAnimationMix(const nArray<nString>& AnimNames, const nArray<float>& Weights,
									  nTime FadeIn, nTime Offset)
{
	n_assert(AnimNames.Size() >= 1);
	n_assert(Weights.Size() >= 1);
	n_assert(pNCharacter);
	n_assert(pCharacterSet);
	n_assert(pNCharacter->GetSkinAnimator());

	const nString& MappedName = GfxSrv->GetAnimationName(AnimMapping, AnimNames[0]).Name;
	int Idx = pNCharacter->GetSkinAnimator()->GetClipIndexByName(MappedName);
	if (Idx != INVALID_INDEX)
	{
		BaseAnimNames = AnimNames;
		BaseAnimWeights = Weights;
		BaseAnimStarted = GetEntityTime();
		BaseAnimFadeIn = FadeIn;
		if (TimeFactor > 0.0f)
		{
			BaseAnimDuration = (pNCharacter->GetSkinAnimator()->GetClipDuration(Idx) - FadeIn) / TimeFactor;
			if (BaseAnimDuration < 0.0) BaseAnimDuration = 0.0;
		}
		else BaseAnimDuration = 0.0;
		BaseAnimOffset = Offset;

		if (!IsOverlayAnimationActive()) ActivateAnimations(BaseAnimNames, BaseAnimWeights, FadeIn);
		else ActivateNewBaseAnim = true;
	}
	else
	{
#ifdef _DEBUG
		n_printf("CCharEntity::SetBaseAnimationMix:: AnimColumn: %s AnimRow: %s -> Animation %s not found! \n",
			AnimMapping.Get(), AnimNames[0].Get(), MappedName.Get());
#endif
	}
}
//---------------------------------------------------------------------

// Set a new overlay animation. This is usually an one-shot animation, like Bash, Jump, etc...
// After the overlay animation has finished, the current base animation will be re-activated.
// OverrideDuration overrides duration if > 0.0
void CCharEntity::SetOverlayAnimation(const nString& AnimName, nTime FadeIn, nTime OverrideDuration, bool OnlyIfInactive)
{
	n_assert(AnimName.IsValid());

	if (OnlyIfInactive && !OverlayAnimNames.Empty() && OverlayAnimNames[0] == AnimName) return;

	nArray<nString> Anims;
	Anims.Append(AnimName);
	nArray<float> Weights;
	Weights.Append(1.0f);

	SetOverlayAnimationMix(Anims, Weights, FadeIn, OverrideDuration);
}
//---------------------------------------------------------------------

void CCharEntity::SetOverlayAnimationMix(const nArray<nString>& AnimNames, const nArray<float>& Weights,
										 nTime FadeIn, nTime OverrideDuration)
{
	n_assert(AnimNames.Size() >= 1);
	n_assert(Weights.Size() >= 1);
	n_assert(pNCharacter);
	n_assert(pCharacterSet);
	n_assert(pNCharacter->GetSkinAnimator());

	const nString& MappedName = GfxSrv->GetAnimationName(AnimMapping, AnimNames[0]).Name;
	int Idx = pNCharacter->GetSkinAnimator()->GetClipIndexByName(MappedName);
	if (Idx != INVALID_INDEX)
	{
		OverlayAnimNames = AnimNames;
		OverlayAnimWeights = Weights;
		OverlayAnimStarted = GetEntityTime();
		if (OverrideDuration > 0.0) OverlayAnimDuration = OverrideDuration;
		else if (TimeFactor > 0.0f)
		{
			OverlayAnimDuration = (pNCharacter->GetSkinAnimator()->GetClipDuration(Idx) - FadeIn) / TimeFactor;
			if (OverlayAnimDuration < 0.0) BaseAnimDuration = 0.0;
		}
		else OverlayAnimDuration = 0.0;

		RestartOverlayAnim = 1;
		OverlayEndFadeIn = FadeIn;

		ActivateAnimations(OverlayAnimNames, OverlayAnimWeights, FadeIn);
	}
}
//---------------------------------------------------------------------

void CCharEntity::StopOverlayAnimation(nTime FadeIn)
{
	OverlayAnimNames.Clear();
	OverlayAnimWeights.Clear();
	RestartOverlayAnim = 0;
	if (!BaseAnimNames.Empty()) ActivateAnimations(BaseAnimNames, BaseAnimWeights, FadeIn);
}
//---------------------------------------------------------------------

void CCharEntity::ActivateAnimations(const nArray<nString>& AnimNames, const nArray<float>& Weights, nTime FadeIn)
{
	n_assert(AnimNames.Size() >= 1);
	n_assert(Weights.Size() >= 1);
	n_assert(pNCharacter);
	n_assert(pCharacterSet);
	n_assert(pNCharacter->GetSkinAnimator());

	pCharacterSet->ClearClips();
	pCharacterSet->fadeInTime = (float)FadeIn;

	//!!!optimize not to lookup! internal function, can use mapped data from above funcs!
	for (int i = 0; i < AnimNames.Size(); i++)
		if (AnimNames[i].IsValid())
		{
			const nString& MappedName = GfxSrv->GetAnimationName(AnimMapping, AnimNames[i]).Name;
			pCharacterSet->AddClip(MappedName, Weights[i]);
		}
}
//---------------------------------------------------------------------

// HACK NOTE: character entities are rotated 180 degrees for rendering.
void CCharEntity::UpdateRenderContextVariables()
{
	CShapeEntity::UpdateRenderContextVariables();

	if (pNCharacter)
	{
		RenderCtx.GetVariable(HAnimTimeOffset)->SetFloat(IsOverlayAnimationActive() ? (float)BaseAnimOffset : 0.f);
		RenderCtx.GetVariable(HChnRestartAnim)->SetInt(RestartOverlayAnim);
		RestartOverlayAnim = 0;

		// HACK: need to rotate characters by 180 degrees around Y
		matrix44 Tfm(Rotate180); //!!!???optimize to avoid copying?!
		Tfm.mult_simple(Transform);
		RenderCtx.SetTransform(Tfm);
		if (ShadowRenderCtx.IsValid()) ShadowRenderCtx.SetTransform(Tfm);
	}
}
//---------------------------------------------------------------------

// This method checks whether the current overlay animation is over and the base animation must be re-activated.
void CCharEntity::OnRenderBefore()
{
	CShapeEntity::OnRenderBefore();

	if (pNCharacter)
	{
		if (IsOverlayAnimationActive())
		{
			// HACK: overlay animation needs to be stopped slightly before it's computed end time to avoid plopping
			static const nTime OverlayAnimStopBuffer = 0.3; // to assert overlay stopping before animation has finished
			nTime EndTime = OverlayAnimStarted + OverlayAnimDuration - OverlayAnimStopBuffer;
			if (GetEntityTime() >= EndTime)
			{
				StopOverlayAnimation(OverlayEndFadeIn);

				if (ActivateNewBaseAnim && BaseAnimNames.Size() > 0)
				{
					n_assert(BaseAnimWeights.Size() == BaseAnimNames.Size());
					ActivateNewBaseAnim = false;
					ActivateAnimations(BaseAnimNames, BaseAnimWeights, BaseAnimFadeIn);
				}
			}
		}

		// emit animation events
		n_assert(pNCharacter);
		float CurrTime = float(GetEntityTime());
		pNCharacter->EmitAnimEvents(CurrTime - (float)(GfxSrv->EntityTimeSrc->GetFrameTime()) * TimeFactor, CurrTime);
	}

	//if (IsChar3Mode)
	//{
	//	n_assert(pCharacter3Node);
	//	SetLocalBox(pCharacter3Node->GetLocalBox());
	//}
}
//---------------------------------------------------------------------

} // namespace Graphics
