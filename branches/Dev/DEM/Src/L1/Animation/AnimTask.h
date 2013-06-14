#pragma once
#ifndef __DEM_L1_ANIM_TASK_H__
#define __DEM_L1_ANIM_TASK_H__

#include <Scene/NodeController.h>
#include <Data/Params.h>
#include <util/ndictionary.h>

// Encapsulates all the data needed to run animation on the scene object. It feeds the list of
// node controllers with sampled data. It also supports fading (in & out) and reverse playback.
// Use these tasks to mix and sequence animations, perform smooth transitions etc.
// NB: some speed sign based conditions use the fact that
// (a > 0 && (b >= c)) || (a < 0 && (b <= c))   =>  a * (b - c) >= 0 (and comparison type variations)

namespace Events
{
	class CEventDispatcher;
}

namespace Scene
{
	class CSceneNode;
}

namespace Anim
{
typedef Ptr<class CAnimClip> PAnimClip;

class CAnimTask
{
protected:

	void Clear();

public:

	enum EState
	{
		Task_Invalid,
		Task_Starting,
		Task_Running,
		Task_Paused,
		Task_Stopping
	};

	CStrID							ClipID;
	Anim::PAnimClip					Clip;
	nArray<Scene::PNodeController>	Ctlrs; //???or weak refs?
	Events::CEventDispatcher*		pEventDisp;	// For anim events
	Data::PParams					Params;		// For anim events
	EState							State;
	float							CurrTime;
	float							StopTimeBase;

	float							Offset;
	float							Speed;
	DWORD							Priority;
	float							Weight;
	float							FadeInTime;
	float							FadeOutTime;
	bool							Loop;

	CAnimTask(): Ctlrs(1, 2), pEventDisp(NULL) { Ctlrs.Flags.Set(Array_DoubleGrowSize); }
	~CAnimTask() { Clear(); }

	void Update(float FrameTime);
	void SetPause(bool Pause);
	void Stop(float OverrideFadeOutTime = -1.f); // Use negative value to avoid override
};

}

#endif
