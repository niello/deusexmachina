#include "Entity.h"

#include <Game/Property.h>
#include <Game/GameLevel.h>
#include <Game/EntityManager.h>
#include <Data/DataArray.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEntity, Core::CObject);

CEntity::CEntity(CStrID _UID): CEventDispatcher(16), UID(_UID)
{
}
//---------------------------------------------------------------------

CEntity::~CEntity()
{
	n_assert_dbg(IsInactive());
}
//---------------------------------------------------------------------

void CEntity::SetUID(CStrID NewUID)
{
	n_assert(NewUID.IsValid());
	if (UID == NewUID) return;
	UID = NewUID;
}
//---------------------------------------------------------------------

void CEntity::SetLevel(CGameLevel* pNewLevel)
{
	n_assert(IsInactive());
	if (pNewLevel == Level.GetUnsafe()) return;
	Level = pNewLevel;
	if (pNewLevel) Level->Subscribe(NULL, this, &CEntity::OnEvent, &LevelSub);
	else LevelSub = NULL;
}
//---------------------------------------------------------------------

void CEntity::Activate()
{
	n_assert(IsInactive() && Level.IsValid());
	Flags.Set(ChangingActivity);

	FireEvent(CStrID("OnEntityActivated"));
	FireEvent(CStrID("OnPropsActivated")); // Needed for initialization of properties dependent on other properties

	Flags.Set(Active);
	Flags.Clear(ChangingActivity);
}
//---------------------------------------------------------------------

void CEntity::Deactivate()
{
	n_assert(IsActive());
	Flags.Set(ChangingActivity);

	Level->RemoveFromSelection(UID);
	FireEvent(CStrID("OnEntityDeactivated"));

	Flags.Clear(Active | ChangingActivity);
}
//---------------------------------------------------------------------

void CEntity::Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc)
{
	Data::PParams SGAttrs = n_new(Data::CParams);
	Data::PParams InitialAttrs;
	if (!pInitialDesc || !pInitialDesc->Get<Data::PParams>(InitialAttrs, CStrID("Attrs")))
		InitialAttrs = n_new(Data::CParams);
	InitialAttrs->GetDiff(*SGAttrs, Attrs);
	if (SGAttrs->GetCount()) OutDesc.Set(CStrID("Attrs"), SGAttrs);

	Data::PDataArray InitialProps;
	if (pInitialDesc) pInitialDesc->Get<Data::PDataArray>(InitialProps, CStrID("Props"));

	CArray<CProperty*> Props;
	EntityMgr->GetPropertiesOfEntity(UID, Props);
	bool Differs = (Props.GetCount() && !InitialProps.IsValid()) || Props.GetCount() != InitialProps->GetCount();
	if (!Differs && Props.GetCount() && InitialProps.IsValid())
	{
		// Quick difference detection is insufficient, do full comparison
		for (int i = 0; i < Props.GetCount(); ++i)
		{
			int ClassFourCC = (int)Props[i]->GetClassFourCC().Code;
			const CString& ClassName = Props[i]->GetClassName();
			int j;
			for (j = 0; j < InitialProps->GetCount(); ++j)
			{
				Data::CData& InitialProp = InitialProps->Get(j);
				if (InitialProp.IsA<int>())
				{
					if (InitialProp == ClassFourCC) break;
				}
				else if (InitialProp.IsA<CString>())
				{
					if (InitialProp == ClassName) break;
				}
				else Sys::Error("Inappropriate property record type, only string class name and int FourCC are allowed!");
			}
			if (j == InitialProps->GetCount())
			{
				Differs = true;
				break;
			}
		}
	}

	if (Differs)
	{
		Data::PDataArray SGProps = n_new(Data::CDataArray);
		for (int i = 0; i < Props.GetCount(); ++i)
			SGProps->Add((int)Props[i]->GetClassFourCC().Code);
		OutDesc.Set(CStrID("Props"), SGProps);
	}
}
//---------------------------------------------------------------------

bool CEntity::OnEvent(const Events::CEventBase& Event)
{
	CStrID EvID = ((Events::CEvent&)Event).ID;

	if (EvID == CStrID("OnEntitiesLoaded"))
	{
		Activate();
		OK;
	}

	if (EvID == CStrID("OnEntitiesUnloading"))
	{
		Deactivate(); //???if (IsActive())?
		OK;
	}

	return !!DispatchEvent(Event);
}
//---------------------------------------------------------------------

}