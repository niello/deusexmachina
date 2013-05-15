#pragma once
#ifndef __DEM_L2_FOCUS_MANAGER_H__
#define __DEM_L2_FOCUS_MANAGER_H__

#include <Core/RefCounted.h>
#include <Events/Events.h>
#include <Data/StringID.h>

// The CFocusManager singleton object manages the global input and camera focus
// entities. There may only be one input and camera focus entity at any
// time, the input focus entity can be different from the camera focus entity.
// The input focus entity will be the entity which receives input, the camera
// focus entity will be the entity which may manipulate the camera.
// The CFocusManager requires an CEntityManager to iterate through existing
// entities, and works only on game entities, which have the CPropInput
// and/or CPropCamera (or a derived class thereof) attached.
// Please note that an actual focus switch will happen only once per-frame.
// This is to avoid chain-reactions when 2 or more objects per frame
// think they currently have the input focus.
// Based on mangalore FocusManager_(C) 2005 Radon Labs GmbH

namespace Game
{
	typedef Ptr<class CEntity> PEntity;
}

namespace Game
{
#define FocusMgr Game::CFocusManager::Instance()

class CFocusManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	static CFocusManager* Singleton;

	PEntity	InputFocusEntity;
	PEntity	CameraFocusEntity;
	PEntity	NewInputFocusEntity;
	PEntity	NewCameraFocusEntity;

	void SetToNextEntity(bool CameraFocus, bool InputFocus);
	bool SwitchToFirstCameraFocusEntity();

	DECLARE_EVENT_HANDLER(OnFrame, OnFrame);
	DECLARE_EVENT_HANDLER(OnLoad, OnLoad);

public:

	CFocusManager();
	virtual ~CFocusManager();

	static CFocusManager* Instance() { n_assert(Singleton); return Singleton; }

	virtual void	Activate();
	virtual void	Deactivate();

	void			SetFocusEntity(CEntity* pEntity);
	CEntity*		GetFocusEntity() const;
	void			SetFocusToNextEntity() { SetToNextEntity(true, true); }
	void			SetDefaultFocus() { SwitchToFirstCameraFocusEntity(); }

	void			SetInputFocusEntity(Game::CEntity* pEntity);
	CEntity*		GetInputFocusEntity() const { return InputFocusEntity.GetUnsafe(); }
	void			SetInputFocusToNextEntity() { SetToNextEntity(false, true); }

	void			SetCameraFocusEntity(Game::CEntity* pEntity);
	CEntity*		GetCameraFocusEntity() const { return CameraFocusEntity.GetUnsafe(); }
	void			SetCameraFocusToNextEntity() { SetToNextEntity(true, false); }
};

// Sets the input and camera focus to the given pEntity. The pEntity pointer may be 0 to clear
// the input and camera focus. The pEntity must have both a CPropInput and CPropCamera
// attached, otherwise the method will fail.
inline void CFocusManager::SetFocusEntity(CEntity* pEntity)
{
	SetInputFocusEntity(pEntity);
	SetCameraFocusEntity(pEntity);
}
//---------------------------------------------------------------------

inline CEntity* CFocusManager::GetFocusEntity() const
{
	return (CameraFocusEntity.GetUnsafe() == InputFocusEntity.GetUnsafe()) ?
		CameraFocusEntity.GetUnsafe() :
		NULL;
}
//---------------------------------------------------------------------

}

#endif
