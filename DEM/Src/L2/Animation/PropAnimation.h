#pragma once
#ifndef __DEM_L2_PROP_ANIM_H__
#define __DEM_L2_PROP_ANIM_H__

#include <Game/Property.h>
#include <Animation/AnimFwd.h>
#include <Animation/AnimClip.h>
#include <Animation/AnimTask.h>
#include <Scene/NodeControllerStatic.h>
#include <Data/Dictionary.h>

// Animation property manages node animation controllers, clip playback and blending.
// This variation supports bones and mocap clips.

//!!!can write no blend animation property without weights etc!

namespace Scene
{
	class CSceneNode;
}

namespace Prop
{
class CPropSceneNode;

class CPropAnimation: public Game::CProperty
{
	__DeclareClass(CPropAnimation);
	__DeclarePropertyStorage;

private:

	CDict<CStrID, Anim::PAnimClip>			Clips;
	CArray<Anim::CAnimTask>					Tasks;
	CArray<Scene::PNodeControllerStatic>	BasePose; // Captured pose for correct fading and blending

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			InitSceneNodeModifiers(CPropSceneNode& Prop);
	void			TermSceneNodeModifiers(CPropSceneNode& Prop);
	void			AddChildrenToMapping(Scene::CSceneNode* pParent, Scene::CSceneNode* pRoot, CDict<int, CStrID>& Bones);
	void			EnableSI(class CPropScriptable& Prop);
	void			DisableSI(class CPropScriptable& Prop);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame); //???OnMoveBefore?

public:

	CPropAnimation(): Tasks(1, 1) { Tasks.SetKeepOrder(false); }

	int				StartAnim(CStrID ClipID, bool Loop = false, float Offset = 0.f, float Speed = 1.f, DWORD Priority = 0, float Weight = 1.f, float FadeInTime = 0.f, float FadeOutTime = 0.f);
	void			PauseAnim(DWORD TaskID, bool Pause) { Tasks[TaskID].SetPause(Pause); }
	void			StopAnim(DWORD TaskID, float FadeOutTime = -1.f);
	float			GetAnimLength(CStrID ClipID) const;
};

}

#endif
