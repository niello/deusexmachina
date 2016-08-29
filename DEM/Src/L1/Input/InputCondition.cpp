#include "InputCondition.h"

#include <Core/Factory.h>

namespace Input
{
__ImplementClassNoFactory(Input::CInputConditionEvent, Core::CRTTIBaseClass);
__ImplementClassNoFactory(Input::CInputConditionState, Core::CRTTIBaseClass);

CInputConditionEvent* CInputConditionEvent::CreateByType(const char* pType)
{
	CString ClassName("Input::CInputCondition");
	ClassName += pType;
	Core::CRTTIBaseClass* pCondition = Factory->Create(ClassName.CStr());
	if (pCondition->IsA("Input::CInputConditionEvent")) return (CInputConditionEvent*)pCondition;
	n_delete(pCondition);
	return NULL;
}
//---------------------------------------------------------------------

CInputConditionState* CInputConditionState::CreateByType(const char* pType)
{
	CString ClassName("Input::CInputCondition");
	ClassName += pType;
	Core::CRTTIBaseClass* pCondition = Factory->Create(ClassName.CStr());
	if (pCondition->IsA("Input::CInputConditionState")) return (CInputConditionState*)pCondition;
	n_delete(pCondition);
	return NULL;
}
//---------------------------------------------------------------------

}