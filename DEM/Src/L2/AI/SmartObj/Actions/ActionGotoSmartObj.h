#pragma once
#ifndef __DEM_L2_AI_ACTION_GOTO_SMART_OBJ_H__
#define __DEM_L2_AI_ACTION_GOTO_SMART_OBJ_H__

#include <AI/Movement/Actions/ActionGoto.h>
#include <Data/StringID.h>
#include <DetourNavMesh.h> // for PolyCache

// Goto action that sets the destination as a position from where a smart object is
// usable, and also ensures an actor has a proper facing to use it. Implemented as
// a simple FSM with 3 states, which helps an actor to behave intelligently.

namespace Prop
{
	class CPropSmartObject;
}

namespace AI
{

class CActionGotoSmartObj: public CActionGoto
{
	__DeclareClass(CActionGotoSmartObj);

private:

	enum EState
	{
		State_Walk,		// Go to a valid destination from where the target is usable
		State_Face,		// Face a valid direction, if needed
		State_Chance	// When no destination is found, give the world a chance to change before failing an action
	};

	CStrID				TargetID;
	CStrID				ActionID;
	CArray<dtPolyRef>	PolyCache;
	EState				State;
	vector3				PrevTargetPos; //!!!can store old face angle too or even whole matrix44 if orientation matters!
	float				RecoveryTime;

public:

	void			Init(CStrID Target, CStrID Action) { TargetID = Target; ActionID = Action; }
	virtual bool	Activate(CActor* pActor);
	virtual UPTR	Update(CActor* pActor);
	virtual void	Deactivate(CActor* pActor);

	virtual void	GetDebugString(CString& Out) const { Out.Format("%s(%s, %s)", GetClassName().CStr(), TargetID.CStr(), ActionID.CStr()); }
};

typedef Ptr<CActionGotoSmartObj> PActionGotoSmartObj;

}

#endif