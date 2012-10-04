#pragma once
#ifndef __DEM_L2_AI_ACTION_FACE_H__
#define __DEM_L2_AI_ACTION_FACE_H__

#include <AI/Behaviour/Action.h>
#include <Data/StringID.h>

// Face action makes actor face specified direction (Actor->FaceDir).
// This direction must be set externally or by derived class.

namespace AI
{

class CActionFace: public CAction
{
	__DeclareClass(CActionFace);

private:

public:

	//virtual bool		Activate(CActor* pActor);
	virtual EExecStatus	Update(CActor* pActor);
	virtual void		Deactivate(CActor* pActor);
	//virtual bool		IsValid(CActor* pActor) const;
};

RegisterFactory(CActionFace);

typedef Ptr<CActionFace> PActionFace;

}

#endif