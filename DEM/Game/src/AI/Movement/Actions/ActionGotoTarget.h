#pragma once
#include "ActionGoto.h"
#include <Data/StringID.h>

// Goto action that sets the destination from the static or dynamic target object

namespace AI
{

class CActionGotoTarget: public CActionGoto
{
	FACTORY_CLASS_DECL;

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
