#pragma once
#include <AI/Behaviour/Action.h> //Sequence.h>
#include <Math/Vector2.h>

// Wander action switches actor between moving in certain radius and standing still,
// sometimes facing random directions.

namespace AI
{

class CActionWander: public CAction //Sequence
{
	FACTORY_CLASS_DECL;

protected:

	vector2	InitialPos;
	float	NextActSelectioCTime;
	PAction	CurrAction;

	bool SelectAction(CActor* pActor);

public:

	//void			Init(action probabilities, timings, wander radius etc);

	virtual bool	Activate(CActor* pActor);
	virtual UPTR	Update(CActor* pActor);
	virtual void	Deactivate(CActor* pActor);

	//virtual bool	IsValid() const { return ppCurrChild && (*ppCurrChild)->IsValid(); }
};

typedef Ptr<CActionWander> PActionWander;

}
