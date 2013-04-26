#pragma once
#ifndef __DEM_L2_PROP_ANIM_H__
#define __DEM_L2_PROP_ANIM_H__

#include <Game/Property.h>
#include <Animation/Anim.h>
#include <Animation/AnimClip.h>
#include <DB/AttrID.h>
#include <util/ndictionary.h>

// Animation property manages node animation controllers, clip playback and blending.
// This variation supports bones and mocap clips.

namespace Attr
{
	DeclareString(AnimDesc);
}

namespace Scene
{
	class CSceneNode;
	class CAnimController;
}

namespace Properties
{

class CPropAnimation: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropAnimation);
	DeclarePropertyStorage;
	DeclarePropertyPools(Game::LivePool);

private:

	enum ETaskState
	{
		Task_Starting,
		Task_Running,
		Task_Paused,
		Task_Stopping
	};

	struct CAnimTask
	{
		typedef nArray<Scene::CAnimController*> CCtlrList;

		CStrID			ClipID;
		Anim::PAnimClip	Clip;
		CCtlrList		Ctlrs;
		ETaskState		State;
		float			CurrTime;
		float			StopTimeBase;

		float			Offset;
		float			Speed;
		DWORD			Priority;
		float			Weight;
		float			FadeInTime;
		float			FadeOutTime;
		bool			Loop;
	};

	nDictionary<CStrID, Anim::PAnimClip>	Clips;
	nDictionary<CStrID, Scene::CSceneNode*>	Nodes; //???where to store blend controller ref?
	nArray<CAnimTask>						Tasks;

	void			AddChildrenToMapping(Scene::CSceneNode* pParent, Scene::CSceneNode* pRoot, nDictionary<int, CStrID>& Bones);

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame); //???OnMoveBefore?

public:

	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void	Activate();
	virtual void	Deactivate();

	int				StartAnim(CStrID ClipID, bool Loop = false, float Offset = 0.f, float Speed = 1.f, DWORD Priority = 0, float Weight = 1.f, float FadeInTime = 0.f, float FadeOutTime = 0.f);
	void			PauseAnim(DWORD TaskID, bool Pause);
	void			StopAnim(DWORD TaskID, float FadeOutTime = -1.f); // Use negative value to avoid override
	float			GetAnimLength(CStrID ClipID) const;
};

RegisterFactory(CPropAnimation);

}

#endif
