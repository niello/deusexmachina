#pragma once
#include "ActionGoto.h"

// Goto action that sets the destination from the predefined world position

namespace AI
{

class CActionGotoPosition: public CActionGoto
{
	FACTORY_CLASS_DECL;

private:

	vector3 Pos;

public:

	void			Init(const vector3& TargetPos) { Pos = TargetPos; }
	virtual bool	Activate(CActor* pActor);
};

typedef Ptr<CActionGotoPosition> PActionGotoPosition;

}
