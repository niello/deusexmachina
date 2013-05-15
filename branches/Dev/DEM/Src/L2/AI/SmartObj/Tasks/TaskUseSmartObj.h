#pragma once
#ifndef __DEM_L2_AI_TASK_USE_SMART_OBJ_H__
#define __DEM_L2_AI_TASK_USE_SMART_OBJ_H__

#include <AI/Behaviour/Task.h>

// Generic 'use smart object' task, makes actor perform action on SO.

namespace Properties
{
	class CPropSmartObject;
}

namespace AI
{
using namespace Properties;

class CTaskUseSmartObj: public CTask
{
	__DeclareClass(CTaskUseSmartObj);

protected:

	CPropSmartObject*	pSO;
	CStrID				ActionID;

public:

	CTaskUseSmartObj(): pSO(NULL) {}

	virtual bool	IsAvailableTo(const CActor* pActor);
	virtual PAction	BuildPlan();

	void			SetSmartObj(CPropSmartObject* pSmartObj) { n_assert(!pSO && pSmartObj); pSO = pSmartObj; }
	void			SetActionID(CStrID ID) { ActionID = ID; }
};

__RegisterClassInFactory(CTaskUseSmartObj);

typedef Ptr<CTaskUseSmartObj> PTaskUseSmartObj;

}

#endif