#pragma once
#include <Game/Property.h>
#include <Animation/AnimFwd.h>
#include <Animation/AnimClip.h>
#include <Animation/AnimTask.h>
#include <Scene/NodeControllerStatic.h>
#include <Data/Dictionary.h>

// Animation property manages node animation controllers, clip playback and blending.
// This variation supports bones and mocap clips.

namespace Scene
{
	class CSceneNode;
}

namespace Prop
{
class CPropSceneNode;

class CPropAnimation: public Game::CProperty
{
	FACTORY_CLASS_DECL;
	__DeclarePropertyStorage;

private:

	struct CTask
	{
		Anim::CAnimTask	AnimTask;
		CStrID			ClipID;
		bool			ManualControl;
		bool			Paused;
	};

	CDict<CStrID, Anim::PAnimClip>			Clips;
	CArray<CTask>							Tasks;
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
	DECLARE_EVENT_HANDLER(BeforeTransforms, BeforeTransforms);

public:

	CPropAnimation(): Tasks(1, 1) { Tasks.SetKeepOrder(false); }

	IPTR			StartAnim(CStrID ClipID, bool Loop = false, float CursorOffset = 0.f, float Speed = 1.f, bool ManualControl = false, UPTR Priority = AnimPriority_Default, float Weight = 1.f, float FadeInTime = 0.f, float FadeOutTime = 0.f);
	void			PauseAnim(UPTR TaskID, bool Pause) { if (TaskID < Tasks.GetCount()) Tasks[TaskID].Paused = Pause; }
	void			StopAnim(UPTR TaskID, float FadeOutTime = -1.f);
	void			SetAnimCursorPos(UPTR TaskID, float Pos);
	bool			SetPose(CStrID ClipID, float CursorPos, bool WrapPos = false) const;
	float			GetAnimLength(CStrID ClipID) const;
};

inline void CPropAnimation::SetAnimCursorPos(UPTR TaskID, float Pos)
{
	if (TaskID < Tasks.GetCount() && Tasks[TaskID].ManualControl)
		Tasks[TaskID].AnimTask.SetCursorPos(Pos);
}
//---------------------------------------------------------------------

}
