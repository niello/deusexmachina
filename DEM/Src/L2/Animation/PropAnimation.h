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
		Task_Active,
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

		float			Speed;
		float			FadeInTime;
		float			FadeOutTime;
		//DWORD Priority, float Weight

		float			CurrTime;

		bool			Loop;
	};

	nDictionary<CStrID, Anim::PAnimClip>			Clips;
	nDictionary<Anim::CBoneID, Scene::CSceneNode*>	Nodes; //???where to store blend controller ref?

	nArray<CAnimTask>								Tasks;
	// Anim task list with IDs. Dictionary or array of slots with freeing slots.
	// Can grow array by 1 and allocate 1 slot at the beginning, since it is not
	// too frequent operation to add new animation tasks.

	void			AddChildrenToMapping(Scene::CSceneNode* pNode);

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame); //???OnMoveBefore?

public:

	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void	Activate();
	virtual void	Deactivate();

	int				StartAnim(CStrID ClipID, bool Loop, float Offset, float Speed, DWORD Priority, float Weight, float FadeInTime, float FadeOutTime);
	void			PauseAnim(DWORD TaskID, bool Pause);
	void			StopAnim(DWORD TaskID, float FadeOutTime);
};

RegisterFactory(CPropAnimation);

}

#endif
