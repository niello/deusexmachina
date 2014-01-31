#pragma once
#ifndef __DEM_L2_AI_ACTION_GOTO_SMART_OBJ_H__
#define __DEM_L2_AI_ACTION_GOTO_SMART_OBJ_H__

#include <AI/Movement/Actions/ActionGoto.h>
#include <Data/StringID.h>
#include <DetourNavMesh.h> // for PolyCache

// Action that makes actor go to a position from where a smart object is usable,
// and also ensures an actor has a proper facing to use it.

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

	CStrID				TargetID;
	CStrID				ActionID;
	CArray<dtPolyRef>	PolyCache;
	bool				IsFacing;

public:

	void			Init(CStrID Target, CStrID Action) { TargetID = Target; ActionID = Action; }
	virtual bool	Activate(CActor* pActor);
	virtual DWORD	Update(CActor* pActor);
	virtual void	Deactivate(CActor* pActor);

	virtual void	GetDebugString(CString& Out) const { Out.Format("%s(%s, %s)", GetClassName().CStr(), TargetID.CStr(), ActionID.CStr()); }
};

typedef Ptr<CActionGotoSmartObj> PActionGotoSmartObj;

}

#endif