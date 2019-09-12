#pragma once
#ifndef __DEM_L1_ANIM_TASK_H__
#define __DEM_L1_ANIM_TASK_H__

#include <Scene/NodeController.h>
#include <Data/Params.h>
#include <Data/Dictionary.h>

// Encapsulates all the data needed to run animation on the scene object. It feeds the list of
// node controllers with sampled data. It also supports fading (in & out) and reverse playback.
// Use these tasks to mix and sequence animations, perform smooth transitions etc.
// NB: tasks don't use real time, they use animation cursor position instead. Cursor pos is
// expressed in the scale of animation clip duration. So, it isn't scaled by speed etc.
// NB: some sign-based conditions use the fact that
// (a > 0 && (b >= c)) || (a < 0 && (b <= c)) __IS__  a * (b - c) >= 0 (with comparison type variations)

namespace Events
{
	class CEventDispatcher;
}

namespace Anim
{
typedef Ptr<class CAnimClip> PAnimClip;

class CAnimTask
{
public:

	enum EState
	{
		Task_Invalid,	// Task is empty and can't be executed
		Task_Starting,	// Task is initialized but not yet started
		Task_Running,	// Non-loop animation or loop fade-out is played
		Task_Looping,	// Loop fade-in or main part is played (prevents fading out)
		Task_LastFrame	// Animation time reached the final point, but controllers must update nodes, so give them a chance
	};

protected:

	Anim::PAnimClip	Clip;
	bool			Loop;
	float			Offset;
	float			Speed;
	float			Weight;

	float			FadeInEndPos;	// Cursor pos at which fading in must end
	float			FadeOutLength;	// Duration of fading out in a clip time scale
	float			FadeOutStartPos;	// Cursor pos at which stopping must begin

	EState			State;
	float			CursorPos;
	float			NewCursorPos;
	float			PrevRealWeight;

	void Clear();

public:

	CArray<Scene::PNodeController>	Ctlrs;		// Is set externally, made public to avoid array copying
	Events::CEventDispatcher*		pEventDisp;	// For anim events
	Data::PParams					Params;		// For anim events

	CAnimTask(): Ctlrs(1, 2), pEventDisp(nullptr), State(Task_Invalid) { Ctlrs.Flags.Set(Array_DoubleGrowSize); }
	~CAnimTask() { Clear(); }

	void	Init(Anim::PAnimClip _Clip, bool _Loop, float _Offset, float _Speed, float _Weight, float FadeInTime, float FadeOutTime);
	void	Update();
	void	Stop(float OverrideFadeOutTime = -1.f); // Use negative value to avoid override

	void	MoveCursorPos(float PosDiff) { NewCursorPos = CursorPos + PosDiff; }
	void	SetCursorPos(float Pos) { NewCursorPos = Pos; }
	float	GetSpeed() const { return Speed; }
	bool	IsEmpty() const { return State == Task_Invalid; }
};

}

#endif
