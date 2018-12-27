#pragma once
#ifndef __DEM_L2_AI_ACTION_GOTO_TARGET_H__
#define __DEM_L2_AI_ACTION_GOTO_TARGET_H__

#include "ActionGoto.h"
#include <Data/StringID.h>

// Goto action that sets the destination from the static or dynamic target object

namespace AI
{

class CActionGotoTarget: public CActionGoto
{
	__DeclareClass(CActionGotoTarget);

private:

	CStrID	TargetID;
	bool	IsDynamic; //!!!no need, check position change!

public:

	void			Init(CStrID Target) { TargetID = Target; }
	virtual bool	Activate(CActor* pActor);
	virtual UPTR	Update(CActor* pActor);
};

typedef Ptr<CActionGotoTarget> PActionGotoTarget;

}

#endif