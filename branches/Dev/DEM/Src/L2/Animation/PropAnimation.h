#pragma once
#ifndef __DEM_L2_PROP_ANIM_H__
#define __DEM_L2_PROP_ANIM_H__

#include <Game/Property.h>
#include <Animation/AnimFwd.h>
#include <Animation/AnimClip.h>
#include <Animation/AnimTask.h>
#include <util/ndictionary.h>

// Animation property manages node animation controllers, clip playback and blending.
// This variation supports bones and mocap clips.

namespace Scene
{
	class CSceneNode;
	class CNodeController;
}

namespace Prop
{
class CPropSceneNode;

class CPropAnimation: public Game::CProperty
{
	__DeclareClass(CPropAnimation);
	__DeclarePropertyStorage;

private:

	//typedef nDictionary<Scene::CSceneNode*, Anim::PNodeControllerPriorityBlend> CCtlrList;

	//???move this cache to CSceneNode? any Prop that manages node controlers may want to access it!
	nDictionary<CStrID, Scene::CSceneNode*>	Nodes;
	//CCtlrList								BlendCtlrs;
	nDictionary<CStrID, Anim::PAnimClip>	Clips;
	nArray<Anim::CAnimTask>					Tasks;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			InitSceneNodeModifiers(CPropSceneNode& Prop);
	void			TermSceneNodeModifiers(CPropSceneNode& Prop);
	void			AddChildrenToMapping(Scene::CSceneNode* pParent, Scene::CSceneNode* pRoot, nDictionary<int, CStrID>& Bones);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame); //???OnMoveBefore?

public:

	CPropAnimation(): Tasks(1, 1) {}

	int				StartAnim(CStrID ClipID, bool Loop = false, float Offset = 0.f, float Speed = 1.f, DWORD Priority = 0, float Weight = 1.f, float FadeInTime = 0.f, float FadeOutTime = 0.f);
	void			PauseAnim(DWORD TaskID, bool Pause) { Tasks[TaskID].SetPause(Pause); }
	void			StopAnim(DWORD TaskID, float FadeOutTime = -1.f) { Tasks[TaskID].Stop(FadeOutTime); }
	float			GetAnimLength(CStrID ClipID) const;
};

}

#endif
