#include "FocusManager.h"

#include <Input/Prop/PropInput.h>
#include <Camera/Prop/PropCamera.h>
#include <Game/EntityManager.h>
#include <Events/EventManager.h>
#include <Input/InputServer.h>
#include <Loading/LoaderServer.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineStrID(InputFocus);
	DefineStrID(CameraFocus);
}

BEGIN_ATTRS_REGISTRATION(FocusManager)
	RegisterStrID(InputFocus, ReadWrite);
	RegisterStrID(CameraFocus, ReadWrite);
END_ATTRS_REGISTRATION

namespace Game
{
__ImplementClassNoFactory(Game::CFocusManager, Core::CRefCounted);

CFocusManager* CFocusManager::Singleton = NULL;

using namespace Properties;

CFocusManager::CFocusManager()
{
	n_assert(!Singleton);
	Singleton = this;
}
//---------------------------------------------------------------------

CFocusManager::~CFocusManager()
{
	n_assert(!InputFocusEntity.IsValid());
	n_assert(!CameraFocusEntity.IsValid());
	n_assert(!NewInputFocusEntity.IsValid());
	n_assert(!NewCameraFocusEntity.IsValid());
	n_assert(Singleton);
	Singleton = NULL;
}
//---------------------------------------------------------------------

void CFocusManager::Activate()
{
	SUBSCRIBE_PEVENT(OnFrame, CFocusManager, OnFrame);
	SUBSCRIBE_PEVENT(OnLoad, CFocusManager, OnLoad);
	if (!LoaderSrv->HasGlobal(Attr::InputFocus)) LoaderSrv->SetGlobal<CStrID>(Attr::InputFocus, CStrID::Empty);
	if (!LoaderSrv->HasGlobal(Attr::CameraFocus)) LoaderSrv->SetGlobal<CStrID>(Attr::CameraFocus, CStrID::Empty);
}
//---------------------------------------------------------------------

void CFocusManager::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnFrame);
	UNSUBSCRIBE_EVENT(OnLoad);
}
//---------------------------------------------------------------------

// Actually switch focus entities. A focus entity switch doesn't happen immediately, but only once per frame.
// This is to prevent chain-reactions and circular reactions when 2 or more entities think they have the
// focus in a single frame.
bool CFocusManager::OnFrame(const Events::CEventBase& Event)
{
	if (NewInputFocusEntity.IsValid())
	{
		n_assert(NewInputFocusEntity->HasProperty<CPropInput>());
		InputFocusEntity = NewInputFocusEntity;
		NewInputFocusEntity = NULL;
#ifndef _EDITOR
		LoaderSrv->SetGlobal(Attr::InputFocus, (CStrID)InputFocusEntity->GetUID());
#endif
		InputFocusEntity->FireEvent(CStrID("OnObtainInputFocus"));
	}

	if (NewCameraFocusEntity.IsValid())
	{
		n_assert(NewCameraFocusEntity->HasProperty<CPropCamera>());
		CameraFocusEntity = NewCameraFocusEntity;
		NewCameraFocusEntity = NULL;
#ifndef _EDITOR
		LoaderSrv->SetGlobal(Attr::CameraFocus, (CStrID)CameraFocusEntity->GetUID());
#endif
		CameraFocusEntity->FireEvent(CStrID("OnObtainCameraFocus"));
	}

	OK;
}
//---------------------------------------------------------------------

bool CFocusManager::OnLoad(const Events::CEventBase& Event)
{
#ifndef _EDITOR
	SetInputFocusEntity(EntityMgr->GetEntity(LoaderSrv->GetGlobal<CStrID>(Attr::InputFocus), true));
	SetCameraFocusEntity(EntityMgr->GetEntity(LoaderSrv->GetGlobal<CStrID>(Attr::CameraFocus), true));
#endif
	OK;
}
//---------------------------------------------------------------------

void CFocusManager::SetToNextEntity(bool CameraFocus, bool InputFocus)
{
	n_assert(CameraFocus || InputFocus);

	const nArray<PEntity>& Entities = EntityMgr->GetEntities();

	nArray<PEntity>::iterator Iter = Entities.Begin();
	if (CameraFocus)
	{
		if (CameraFocusEntity.IsValid()) Iter = Entities.Find(CameraFocusEntity);
	}
	else if (InputFocusEntity.IsValid()) Iter = Entities.Find(InputFocusEntity);

	nArray<PEntity>::iterator Start = Iter;
    if (Iter) do
    {
		Iter++;
		if (Iter == Entities.End()) Iter = Entities.Begin();

		CEntity* pEntity = *Iter;
		bool HasCameraProp = pEntity->HasProperty<CPropCamera>();
		bool HasInputProp  = pEntity->HasProperty<CPropInput>();
		if (CameraFocus && InputFocus && HasCameraProp && HasInputProp)
		{
			SetFocusEntity(pEntity);
			return;
		}
		else if (CameraFocus && (!InputFocus) && HasCameraProp)
		{
			SetCameraFocusEntity(pEntity);
			return;
		}
		else if (InputFocus && (!CameraFocus) && HasInputProp)
		{
			SetInputFocusEntity(pEntity);
			return;
		}
	}
	while (Iter != Start);
}
//---------------------------------------------------------------------

bool CFocusManager::SwitchToFirstCameraFocusEntity()
{
	const nArray<PEntity>& Entities = EntityMgr->GetEntities();
	nArray<PEntity>::iterator Iter = Entities.Begin();

	while (Iter != Entities.End())
	{
		CEntity* pEntity = *Iter;
		if (pEntity->HasProperty<CPropCamera>())
		{
			SetCameraFocusEntity(pEntity);
			//???try to set input?
			OK;
		}
		Iter++;
	}

	FAIL;
}
//---------------------------------------------------------------------

// Set input focus pEntity to the given pEntity. The pEntity pointer can be NULL, this will clear the
// current input focus. The pEntity must have an CPropInput attached for this to work.
void CFocusManager::SetInputFocusEntity(CEntity* pEntity)
{
	if (InputFocusEntity.GetUnsafe() == pEntity) return;
	if (InputFocusEntity.IsValid()) InputFocusEntity->FireEvent(CStrID("OnLoseInputFocus"));
	InputFocusEntity = NULL;
	NewInputFocusEntity = pEntity;
}
//---------------------------------------------------------------------

// Set camera focus pEntity to the given pEntity. The pEntity pointer can be NULL, this will clear the
// current camera focus. The pEntity must have a CPropCamera attached for this to work.
void CFocusManager::SetCameraFocusEntity(CEntity* pEntity)
{
	if (CameraFocusEntity.GetUnsafe() == pEntity) return;
	if (CameraFocusEntity.IsValid()) CameraFocusEntity->FireEvent(CStrID("OnLoseCameraFocus"));
	CameraFocusEntity = NULL;
	NewCameraFocusEntity = pEntity;
}
//---------------------------------------------------------------------

} // namespace Game
