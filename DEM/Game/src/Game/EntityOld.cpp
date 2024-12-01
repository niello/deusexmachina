#include "Entity.h"

#include <Game/Property.h>
#include <Game/GameLevel.h>
#include <Data/DataArray.h>

namespace Game
{

CEntity::CEntity(CStrID _UID): UID(_UID), Flags(WaitingForLevelActivation)
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
	if (pNewLevel == Level.Get()) return;
	Level = pNewLevel;
	LevelSub = pNewLevel ? pNewLevel->Subscribe(nullptr, this, &CEntity::OnEvent) : ::Events::PSub {};
}
//---------------------------------------------------------------------

CGameLevel* CEntity::GetLevel() const
{
	return Level.Get();
}
//---------------------------------------------------------------------

void CEntity::Activate()
{
	n_assert(IsInactive() && Level.IsValidPtr());
	Flags.Set(ChangingActivity);

	FireEvent(CStrID("OnActivated"));
	FireEvent(CStrID("OnPropsActivated")); // Needed for initialization of properties dependent on other properties

	Flags.Set(Active);
	Flags.Clear(ChangingActivity | WaitingForLevelActivation);
}
//---------------------------------------------------------------------

void CEntity::Deactivate()
{
	n_assert(IsActive());
	Flags.Set(ChangingActivity);

	FireEvent(CStrID("OnDeactivated"));

	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("EntityID"), UID);
	Level->FireEvent(CStrID("OnEntityDeactivated"), P);

	Flags.Clear(Active | ChangingActivity);
}
//---------------------------------------------------------------------

void CEntity::Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc)
{
	Data::PParams SGAttrs = n_new(Data::CParams);
	Data::PParams InitialAttrs;
	if (!pInitialDesc || !pInitialDesc->TryGet<Data::PParams>(InitialAttrs, CStrID("Attrs")))
		InitialAttrs = n_new(Data::CParams);
	InitialAttrs->GetDiff(*SGAttrs, Attrs);
	if (SGAttrs->GetCount()) OutDesc.Set(CStrID("Attrs"), SGAttrs);

	Data::PDataArray InitialProps;
	if (pInitialDesc) pInitialDesc->TryGet<Data::PDataArray>(InitialProps, CStrID("Props"));

	CArray<CProperty*> Props;
	//GameSrv->GetEntityMgr()->GetPropertiesOfEntity(UID, Props);
	bool Differs = (Props.GetCount() && InitialProps.IsNullPtr()) || Props.GetCount() != InitialProps->GetCount();
	if (!Differs && Props.GetCount() && InitialProps.IsValidPtr())
	{
		// Quick difference detection is insufficient, do full comparison
		for (UPTR i = 0; i < Props.GetCount(); ++i)
		{
			int ClassFourCC = (int)Props[i]->GetClassFourCC().Code;
			const auto& ClassName = Props[i]->GetClassName();
			UPTR j;
			for (j = 0; j < InitialProps->GetCount(); ++j)
			{
				Data::CData& InitialProp = InitialProps->Get(j);
				if (InitialProp.IsA<int>())
				{
					if (InitialProp == ClassFourCC) break;
				}
				else if (InitialProp.IsA<CString>())
				{
					if (InitialProp.GetValue<CString>().CStr() == ClassName) break;
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
		for (UPTR i = 0; i < Props.GetCount(); ++i)
			SGProps->Add((int)Props[i]->GetClassFourCC().Code);
		OutDesc.Set(CStrID("Props"), SGProps);
	}
}
//---------------------------------------------------------------------

bool CEntity::OnEvent(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (Event.IsA<Events::CEvent>())
	{
		CStrID EvID = ((Events::CEvent&)Event).ID;

		// Internal event of the level
		if (Flags.Is(WaitingForLevelActivation) && EvID == CStrID("ValidateEntities"))
		{
			Activate();
			OK;
		}

		// Internal event of the level
		if (EvID == CStrID("OnEntitiesUnloading"))
		{
			Deactivate(); //???if (IsActive())?
			OK;
		}
	}

	return !!FireEvent(Event);
}
//---------------------------------------------------------------------

}
