#include "ControlLayout.h"

#include <Events/EventServer.h>

namespace Input
{

bool CControlLayout::Initialize(const Data::CParams& Desc)
{
	Data::PParams Mappings;

	/*
	EventMappings.Clear();
	if (Desc.Get<Data::PParams>(Mappings, CStrID("Events")) && Mappings->GetCount())
	{
		CInputMappingEvent* pNew = EventMappings.Reserve(Mappings->GetCount());
		for (UPTR i = 0; i < Mappings->GetCount(); ++i, ++pNew)
		{
			Data::CParam& Prm = Mappings->Get(i);
			if (Prm.IsA<int>())
			{
				// Syntax shortcut for KeyUp events
				if (!pNew->Init(Prm.GetName(), Prm.GetValue<int>())) FAIL;
			}
			else if (!pNew->Init(Prm.GetName(), *Prm.GetValue<Data::PParams>())) FAIL;
		}
	}

	StateMappings.Clear();
	if (Desc.Get<Data::PParams>(Mappings, CStrID("States")) && Mappings->GetCount())
	{
		CInputMappingState* pNew = StateMappings.Reserve(Mappings->GetCount());
		for (UPTR i = 0; i < Mappings->GetCount(); ++i, ++pNew)
		{
			Data::CParam& Prm = Mappings->Get(i);
			if (!pNew->Init(Prm.GetName(), *Prm.GetValue<Data::PParams>())) FAIL;
		}
	}

	Enabled = false;
	*/

	OK;
}
//---------------------------------------------------------------------

void CControlLayout::Reset()
{
	for (CDict<CStrID, CInputConditionState*>::CIterator It = States.Begin(); It != States.End(); ++It)
		(*It).GetValue()->Reset();
	for (CArray<CInputConditionEvent*>::CIterator It = Events.Begin(); It != Events.End(); ++It)
		(*It)->Reset();
}
//---------------------------------------------------------------------

}