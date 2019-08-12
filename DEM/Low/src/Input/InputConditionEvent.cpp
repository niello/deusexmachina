#include "InputConditionEvent.h"
#include <Core/Factory.h>

namespace Input
{
__ImplementClassNoFactory(Input::CInputConditionEvent, Core::CRTTIBaseClass);

CInputConditionEvent* CInputConditionEvent::CreateByType(const char* pType)
{
	CString ClassName("Input::CInputCondition");
	ClassName += pType;
	Core::CRTTIBaseClass* pCondition = Factory->Create(ClassName.CStr());
	if (auto pConditionEvent = pCondition->As<CInputConditionEvent>())
		return pConditionEvent;
	n_delete(pCondition);
	return nullptr;
}
//---------------------------------------------------------------------

}