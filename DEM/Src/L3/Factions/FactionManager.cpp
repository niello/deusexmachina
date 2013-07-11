#include "FactionManager.h"

#include <Scripting/ScriptServer.h>
#include <Events/EventManager.h>

namespace RPG
{
__ImplementClassNoFactory(RPG::CFactionManager, Core::CRefCounted);
__ImplementSingleton(RPG::CFactionManager);

CFactionManager::CFactionManager()
{
	__ConstructSingleton;

	SUBSCRIBE_PEVENT(OnGameDescLoaded, CFactionManager, OnGameDescLoaded);
	SUBSCRIBE_PEVENT(OnGameSaving, CFactionManager, OnGameSaving);
}
//---------------------------------------------------------------------

CFactionManager::~CFactionManager()
{
	UNSUBSCRIBE_EVENT(OnGameDescLoaded);
	UNSUBSCRIBE_EVENT(OnGameSaving);

	//???!!!make more implicit and/or safe!?
	for (int i = 0; i < Factions.GetCount(); ++i)
		ScriptSrv->RemoveObject(Factions.KeyAt(i).CStr(), "Factions");
	Factions.Clear();

	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CFactionManager::OnGameDescLoaded(const Events::CEventBase& Event)
{
	//???!!!make more implicit and/or safe!?
	for (int i = 0; i < Factions.GetCount(); ++i)
		ScriptSrv->RemoveObject(Factions.KeyAt(i).CStr(), "Factions");
	Factions.Clear();

	Data::PParams GameDesc = ((const Events::CEvent&)Event).Params;
	Data::PParams SGFactions;
	if (!GameDesc->Get(SGFactions, CStrID("Factions")) || !SGFactions->GetCount()) OK;

	Factions.BeginAdd();
	for (int i = 0; i < SGFactions->GetCount(); ++i)
	{
		Data::CParam& Prm = SGFactions->Get(i);
		Data::PParams FactionDesc = Prm.GetValue<Data::PParams>();

		PFaction Faction = Factions.Add(Prm.GetName(), n_new(CFaction));

		//!!!???Faction->Load()? good way to load without events

		n_verify(ScriptSrv->CreateInterface(Prm.GetName().CStr(), "Factions", "CFaction", Faction.GetUnsafe()));

		//!!!without OnFactionMemberAdopted etc events!

		Data::PParams Members;
		if (FactionDesc->Get(Members, CStrID("Members")))
			for (int j = 0; j < Members->GetCount(); ++j)
			{
				Data::CParam& MemberRec = Members->Get(j);
				Faction->AdoptMember(MemberRec.GetName(), MemberRec.GetValue<int>());
			}

		//!!!load attitudes!
	}
	Factions.EndAdd();

	OK;
}
//---------------------------------------------------------------------

bool CFactionManager::OnGameSaving(const Events::CEventBase& Event)
{
	Data::PParams SGCommon = ((const Events::CEvent&)Event).Params;

	Data::PParams SGFactions = n_new(Data::CParams);
	for (int i = 0; i < Factions.GetCount(); ++i)
	{
		CFaction* pFaction = Factions.ValueAt(i).GetUnsafe();

		//???to pFaction->Save()?
		Data::PParams SGFaction = n_new(Data::CParams);
		Data::PParams SGMembers = n_new(Data::CParams);
		for (DWORD j = 0; j < pFaction->GetMemberCount(); ++j)
		{
			CStrID MID = pFaction->GetMember(i);
			SGMembers->Set(MID, pFaction->GetMemberRank(MID));
		}
		if (SGMembers->GetCount()) SGFaction->Set(CStrID("Members"), SGMembers);

		if (SGFaction->GetCount()) SGFactions->Set(Factions.KeyAt(i), SGFaction);
	}

	//!!!need diff! so, need initial desc!
	//!!!remember to write "Factions = nil" if NO data!
	//???or global diff once in GameSrv?

	if (SGFactions->GetCount()) SGCommon->Set(CStrID("Factions"), SGFactions);

	OK;
}
//---------------------------------------------------------------------

}