#include "ControlLayout.h"

#include <Data/Params.h>
#include <Data/DataArray.h>

namespace Input
{

bool CControlLayout::Initialize(const Data::CDataArray& Desc)
{
	States.BeginAdd();

	for (UPTR i = 0; i < Desc.GetCount(); ++i)
	{
		Data::PParams MappingDesc = Desc.Get<Data::PParams>(i);

		CString ConditionType = MappingDesc->Get<CString>(CStrID("Type"), CString::Empty);
		if (ConditionType.IsEmpty()) FAIL;

		CString OutputName;
		if (MappingDesc->Get(OutputName, CStrID("Event")))
		{
			CEventRecord& NewRec = *Events.Add();
			NewRec.OutEventID = CStrID(OutputName.CStr());
			NewRec.pEvent = CInputConditionEvent::CreateByType(ConditionType);
			if (!NewRec.pEvent || !NewRec.pEvent->Initialize(*MappingDesc.Get())) FAIL;
		}
		else if (MappingDesc->Get(OutputName, CStrID("State")))
		{
			CInputConditionState* pState = CInputConditionState::CreateByType(ConditionType);
			if (!pState || !pState->Initialize(*MappingDesc.Get())) FAIL;
			States.Add(CStrID(OutputName.CStr()), pState);
		}
		//else FAIL;
	}

	States.EndAdd();

	OK;
}
//---------------------------------------------------------------------

void CControlLayout::Clear()
{
	for (CDict<CStrID, CInputConditionState*>::CIterator It = States.Begin(); It != States.End(); ++It)
		n_delete(It->GetValue());
	States.Clear();
	for (CArray<CEventRecord>::CIterator It = Events.Begin(); It != Events.End(); ++It)
		n_delete(It->pEvent);
	Events.Clear();
}
//---------------------------------------------------------------------

void CControlLayout::Reset()
{
	for (CDict<CStrID, CInputConditionState*>::CIterator It = States.Begin(); It != States.End(); ++It)
		It->GetValue()->Reset();
	for (CArray<CEventRecord>::CIterator It = Events.Begin(); It != Events.End(); ++It)
		It->pEvent->Reset();
}
//---------------------------------------------------------------------

}