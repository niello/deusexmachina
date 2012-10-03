#include "SmartObjActionTpl.h"

#include <Data/Params.h>

namespace AI
{
using namespace Data;

CSmartObjActionTpl::CSmartObjActionTpl(const CParams& Desc)
{
	MinDistance = GetFloat(Desc, CStrID("MinDistance"));
	MaxDistance = GetFloat(Desc, CStrID("MaxDistance"), -1.f);
	//DestOffset = Desc.Get<PDataArray>(CStrID("DestOffset"))->AsVector3(vector3::Zero);
	Flags.SetTo(ACTOR_RADIUS_MATTERS, Desc.Get<bool>(CStrID("ActorRadiusMatters"), true));

	Flags.SetTo(FACE_OBJECT, Desc.Get<bool>(CStrID("Face"), false));
	
	Flags.SetTo(LOOP_ANIM, Desc.Get<bool>(CStrID("LoopAnim"), true));
	Duration = GetFloat(Desc, CStrID("Duration"));

	MaxUserCount = Desc.Get<int>(CStrID("MaxUserCount"), -1);

	Flags.SetTo(END_ON_DONE, Desc.Get<bool>(CStrID("EndOnDone"), true));
	Flags.SetTo(RESET_ON_ABORT, Desc.Get<bool>(CStrID("ResetOnAbort"), true));
}
//---------------------------------------------------------------------

} //namespace AI