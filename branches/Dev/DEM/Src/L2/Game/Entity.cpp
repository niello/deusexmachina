#include "Entity.h"

#include <Game/Property.h>
#include <Game/GameLevel.h>
#include <Game/EntityManager.h>
#include <Data/DataArray.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEntity, Core::CRefCounted);

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
	LevelSub = pNewLevel ? Level->Subscribe(NULL, this, &CEntity::OnEvent) : NULL;
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

	//!!!Save props! Get all props, try to find in initial, if all is equal, skip, else write all!
	nArray<CProperty*> Props;
	EntityMgr->GetPropertiesOfEntity(UID, Props);
	bool Differs = (Props.GetCount() && !InitialProps.IsValid()) || Props.GetCount() != InitialProps->GetCount();
	if (!Differs && Props.GetCount() && InitialProps.IsValid())
	{
		// Quick difference detection is insufficient, do full comparison
		for (int i = 0; i < Props.GetCount(); ++i)
		{
			//???!!!use FourCC!?
			const nString& ClassName = Props[i]->GetClassName();
			int j;
			for (j = 0; j < InitialProps->GetCount(); ++j)
				if (InitialProps->Get<nString>(j) == ClassName)
					break;
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
			SGProps->Append(Props[i]->GetClassName());
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
		Deactivate();
		OK;
	}

	if (EvID == CStrID("OnBeginFrame")) ProcessPendingEvents();
	return !!DispatchEvent(Event);
}
//---------------------------------------------------------------------

} // namespace Game
