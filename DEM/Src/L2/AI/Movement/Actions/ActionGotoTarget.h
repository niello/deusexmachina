#pragma once
#ifndef __DEM_L2_AI_ACTION_GOTO_TARGET_H__
#define __DEM_L2_AI_ACTION_GOTO_TARGET_H__

#include "ActionGoto.h"
#include <Data/StringID.h>

// Goto action that sets destination from static or dynamic target object.

namespace AI
{

class CActionGotoTarget: public CActionGoto
{
	__DeclareClass(CActionGotoTarget);

private:

	CStrID	TargetID;
	bool	IsDynamic;

public:

	void				Init(CStrID Target) { TargetID = Target; }
	virtual bool		Activate(CActor* pActor);
	virtual DWORD	Update(CActor* pActor);
};

typedef Ptr<CActionGotoTarget> PActionGotoTarget;

}

#endif