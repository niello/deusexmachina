#pragma once
#include <AI/Behaviour/Action.h>
#include <Data/StringID.h>

// Face action makes actor face specified direction (Actor->FaceDir).
// This direction must be set externally or by derived class.

namespace AI
{

class CActionFace: public CAction
{
	FACTORY_CLASS_DECL;

public:

	virtual UPTR	Update(CActor* pActor);
	virtual void	Deactivate(CActor* pActor);
};

typedef Ptr<CActionFace> PActionFace;

}
