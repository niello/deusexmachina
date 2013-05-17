#pragma once
#ifndef __DEM_L2_PROP_ANIM_H__
#define __DEM_L2_PROP_ANIM_H__

#include <Game/Property.h>
#include <Animation/Anim.h>
#include <Animation/AnimClip.h>
#include <Animation/AnimTask.h>
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
	__DeclareClass(CPropAnimation);
	__DeclarePropertyStorage;

private:

	nDictionary<CStrID, Anim::PAnimClip>	Clips;
	nDictionary<CStrID, Scene::CSceneNode*>	Nodes; //???where to store blend controller ref?
	nArray<Anim::CAnimTask>					Tasks;

	void			AddChildrenToMapping(Scene::CSceneNode* pParent, Scene::CSceneNode* pRoot, nDictionary<int, CStrID>& Bones);

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame); //???OnMoveBefore?

public:

	CPropAnimation(): Tasks(1, 1) {}

	virtual void	Activate();
	virtual void	Deactivate();

	int				StartAnim(CStrID ClipID, bool Loop = false, float Offset = 0.f, float Speed = 1.f, DWORD Priority = 0, float Weight = 1.f, float FadeInTime = 0.f, float FadeOutTime = 0.f);
	void			PauseAnim(DWORD TaskID, bool Pause) { Tasks[TaskID].SetPause(Pause); }
	void			StopAnim(DWORD TaskID, float FadeOutTime = -1.f) { Tasks[TaskID].Stop(FadeOutTime); }
	float			GetAnimLength(CStrID ClipID) const;
};

__RegisterClassInFactory(CPropAnimation);

}

#endif
