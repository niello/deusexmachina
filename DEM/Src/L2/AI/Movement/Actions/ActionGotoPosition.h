#pragma once
#ifndef __DEM_L2_AI_ACTION_GOTO_POSITION_H__
#define __DEM_L2_AI_ACTION_GOTO_POSITION_H__

#include "ActionGoto.h"

// Goto action that sets the destination from the predefined world position

namespace AI
{

class CActionGotoPosition: public CActionGoto
{
	__DeclareClass(CActionGotoPosition);

private:

	vector3 Pos;

public:

	void			Init(const vector3& TargetPos) { Pos = TargetPos; }
	virtual bool	Activate(CActor* pActor);
};

typedef Ptr<CActionGotoPosition> PActionGotoPosition;

}

#endif