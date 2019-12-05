#pragma once
#include <AI/Movement/Actions/ActionFace.h>

// Face target action makes actor face target entity, which can be dynamic, so actor can track target.

namespace AI
{

class CActionFaceTarget: public CActionFace
{
	FACTORY_CLASS_DECL;

private:

	CStrID	TargetID;
	bool	IsDynamic;

	bool SetupDirFromTarget(CActor* pActor);

public:

	virtual void	Init(const Data::CParams& Desc);
	void			Init(CStrID Target) { TargetID = Target; }

	virtual bool	Activate(CActor* pActor);
	virtual UPTR	Update(CActor* pActor);
};

typedef Ptr<CActionFaceTarget> PActionFaceTarget;

}
