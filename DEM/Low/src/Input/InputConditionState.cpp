#include "InputConditionState.h"
#include <Core/Factory.h>

namespace Input
{
__ImplementClassNoFactory(Input::CInputConditionState, Core::CRTTIBaseClass);

CInputConditionState* CInputConditionState::CreateByType(const char* pType)
{
	CString ClassName("Input::CInputCondition");
	ClassName += pType;
	Core::CRTTIBaseClass* pCondition = Factory->Create(ClassName.CStr());
	if (auto pConditionState = pCondition->As<CInputConditionState>())
		return pConditionState;
	n_delete(pCondition);
	return nullptr;
}
//---------------------------------------------------------------------

}