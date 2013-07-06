#include "FactionManager.h"

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

	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CFactionManager::OnGameDescLoaded(const Events::CEventBase& Event)
{
	//!!!without gameplay events!
	Factions.Clear();

	Data::PParams GameDesc = ((const Events::CEvent&)Event).Params;
	Data::PParams SGFactions;
	if (!GameDesc->Get(SGFactions, CStrID("Factions")) || !SGFactions->GetCount()) OK;

	for (int i = 0; i < SGFactions->GetCount(); ++i)
	{
		//Data::PParams SGFaction = SGFactions->Get(i);

		//CStrID FactionID = SGFaction->Get<CStrID>(CStrID("ID"));

		//!!!without OnFactionMemberAdopted etc events!
		//Add faction, script object, members, attitudes
	}

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