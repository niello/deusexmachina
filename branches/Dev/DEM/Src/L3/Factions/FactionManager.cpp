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

	for (int i = 0; i < Factions.GetCount(); ++i)
		ScriptSrv->RemoveObject(Factions.KeyAt(i).CStr(), "Factions");
	Factions.Clear();

	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CFactionManager::OnGameDescLoaded(const Events::CEventBase& Event)
{
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
	n_error("IMPLEMENT ME!!!");
	//Data::PParams SGCommon = ((const Events::CEvent&)Event).Params;

	//Data::PDataArray SGFactions = n_new(Data::CDataArray);

	//if (SGFactions->GetCount()) SGCommon->Set(CStrID("Factions"), SGFactions);

	OK;
}
//---------------------------------------------------------------------

}