#pragma once
#ifndef __DEM_L2_GFX_CHAR_ENTITY_H__
#define __DEM_L2_GFX_CHAR_ENTITY_H__

#include "ShapeEntity.h"
#include <Gfx/CharAnimEventHandler.h>
#include <character/ncharjoint.h>
#include <character/ncharacter2set.h>
#include <character/ncharacter2.h>

// A graphics entity specialized for rendering animated characters.

namespace Graphics
{

class CCharEntity: public CShapeEntity
{
	DeclareRTTI;
	DeclareFactory(CCharEntity);

private:

	//???can reduce number of variables?

	nArray<nString>		BaseAnimNames;      ///< names of current base animations
	nArray<float>		BaseAnimWeights;      ///< weights of the base animations
	nTime				BaseAnimStarted;              ///< timestamp when base animation has been started
	nTime				BaseAnimOffset;               ///< animation offset for base animation
	nTime				BaseAnimDuration;             ///< duration of one loop of the base animation
	nTime				BaseAnimFadeIn;
	bool				ActivateNewBaseAnim;

	nArray<nString>		OverlayAnimNames;   ///< names of current overlay animations
	nArray<float>		OverlayAnimWeights;   ///< weights of the overlay animations
	nTime				OverlayAnimStarted;           ///< timestamp when overlay animation has been started
	nTime				OverlayAnimDuration;          ///< duration of overlay animation
	int					RestartOverlayAnim;             ///< flag indicating to Nebula2 that animation must be restarted
	nTime				OverlayEndFadeIn;             ///< fade in time when overlay animation has finished

	nVariable::Handle	HCharacter;      ///< handle of character pointer local var in RenderContext
	nVariable::Handle	HCharacterSet;   ///< handle of character set pointer local var in RenderContext
	nVariable::Handle	HAnimTimeOffset;
	nVariable::Handle	HChnRestartAnim;

	nCharacter2*		pNCharacter;
	nCharacter2Set*		pCharacterSet;
	CCharAnimEHandler*	pAnimEventHandler;

	virtual void	UpdateRenderContextVariables();
	void			UpdateTime();
	void			ActivateAnimations(const nArray<nString>& AnimNames, const nArray<float>& Weights, nTime FadeIn);
	int				GetNumAnimations() const;

public:

	nString AnimMapping;

	CCharEntity();
	virtual ~CCharEntity() {}

	virtual void Activate();
	virtual void Deactivate();
	virtual void OnRenderBefore();

	void	EvaluateSkeleton();

	void	SetBaseAnimation(const nString& AnimName, nTime FadeIn = 0.0, nTime Offset = 0.0, bool OnlyIfInactive = true);
	void	SetBaseAnimationMix(const nArray<nString>& AnimNames, const nArray<float>& Weights, nTime FadeIn = 0.0, nTime Offset = 0.0);
	nString	GetBaseAnimation() const { return BaseAnimNames.Empty() ? "" : BaseAnimNames[0]; }
	nTime	GetBaseAnimationDuration() const { return BaseAnimDuration; }
	void	SetOverlayAnimation(const nString& AnimName, nTime FadeIn = 0.0, nTime OverrideDuration = 0.0, bool OnlyIfInactive = true);
	void	SetOverlayAnimationMix(const nArray<nString>& AnimNames, const nArray<float>& Weights, nTime FadeIn = 0.0, nTime OverrideDuration = 0.0f);
	nString	GetOverlayAnimation() const { return OverlayAnimNames.Empty() ? "" : OverlayAnimNames[0]; }
	nTime	GetOverlayAnimationDuration() const { return OverlayAnimDuration; }
	void	StopOverlayAnimation(nTime FadeIn);
	bool	IsOverlayAnimationActive() const { return !OverlayAnimNames.Empty(); }

	int					GetJointIndexByName(const nString& Name);
	nCharJoint*			GetJoint(int Idx) const;
	const matrix44&		GetJointMatrix(int Idx) const;
	CCharAnimEHandler*	GetAnimationEventHandler() const { return pAnimEventHandler; }
	nCharacter2*		GetCharacterPointer() const { return pNCharacter; }
	nCharacter2Set*		GetCharacterSet() const { return pCharacterSet; }
};
//---------------------------------------------------------------------

RegisterFactory(CCharEntity);

// This brings the character's skeleton uptodate. Make sure the entity's time and animation state weights
// in the render context are uptodate before calling this method, to avoid one-frame-latencies.
inline void CCharEntity::EvaluateSkeleton()
{
	if (pNCharacter) pNCharacter->EvaluateSkeleton((float)GetEntityTime());
}
//---------------------------------------------------------------------

inline int CCharEntity::GetJointIndexByName(const nString& Name)
{
	n_assert(Name.IsValid());
	return pNCharacter ? pNCharacter->GetSkeleton().GetJointIndexByName(Name) : INVALID_INDEX;
}
//---------------------------------------------------------------------

inline nCharJoint* CCharEntity::GetJoint(int Idx) const
{
	return pNCharacter ? &(pNCharacter->GetSkeleton().GetJointAt(Idx)) : NULL;
}
//---------------------------------------------------------------------

// Returns a joint's current matrix in model space. Make sure to call EvaluateSkeleton() before!
inline const matrix44& CCharEntity::GetJointMatrix(int Idx) const
{
	return pNCharacter ? pNCharacter->GetSkeleton().GetJointAt(Idx).GetMatrix() : matrix44::identity;
}
//---------------------------------------------------------------------

}

#endif
