#include "SmartAction.h"

namespace AI
{

void CSmartAction::Init(const Data::CParams& Desc)
{
	Flags.SetTo(END_ON_DONE, Desc.Get<bool>(CStrID("EndOnDone"), true));
	Flags.SetTo(RESET_ON_ABORT, Desc.Get<bool>(CStrID("ResetOnAbort"), true));
	Flags.SetTo(MANUAL_TRANSITION, Desc.Get<bool>(CStrID("ManualTransition"), false));
	Flags.SetTo(SEND_PROGRESS_EVENT, Desc.Get<bool>(CStrID("SendProgressEvent"), false));

	MinRange = GetFloat(Desc, CStrID("MinDistance"));
	MaxRange = GetFloat(Desc, CStrID("MaxDistance"), -1.f);
	//DestOffset = Desc.Get<PDataArray>(CStrID("DestOffset"))->AsVector3(vector3::Zero);
	Flags.SetTo(ACTOR_RADIUS_MATTERS, Desc.Get<bool>(CStrID("ActorRadiusMatters"), true));
	Flags.SetTo(FACE_OBJECT, Desc.Get<bool>(CStrID("Face"), false));
	
	Duration = GetFloat(Desc, CStrID("Duration"));
	TargetState = Desc.Get<CStrID>(CStrID("TargetState"), CStrID::Empty);

	CString PD = Desc.Get<CString>(CStrID("ProgressDriver"), NULL);
	PD.TrimInplace();
	PD.ToLower();
	if (PD == "dur" || PD == "duration") ProgressDriver = PDrv_Duration;
	else if (PD == "fsm" || PD == "so.fsm") ProgressDriver = PDrv_SO_FSM;
	else ProgressDriver = PDrv_None;

	MaxUserCount = Desc.Get<int>(CStrID("MaxUserCount"), -1);

	static const CString StrWSSrcPfx("AI::CWorldStateSource");
	Data::PParams SubDesc;
	if (Desc.Get<Data::PParams>(SubDesc, CStrID("Preconditions")))
	{
		Preconditions = (CWorldStateSource*)Factory->Create(StrWSSrcPfx + SubDesc->Get<CString>(CStrID("Type")));
		Preconditions->Init(SubDesc);
	}
	else Preconditions = NULL;

	//// Optional(?) scripted functions
	////???or use callbacks with predefined names and inside differ by action ID?
	//Data::CSimpleString	ValidateFunc;
	//Data::CSimpleString	GetDestinationFunc;
	//Data::CSimpleString	GetDurationFunc;
	//Data::CSimpleString	UpdateFunc;
	//ValidateFunc = Desc.Get<CString>(CStrID("ValidateFunc"), NULL).CStr();
	//GetDestinationFunc = Desc.Get<CString>(CStrID("GetDestinationFunc"), NULL).CStr();
	//GetDurationFunc = Desc.Get<CString>(CStrID("GetDurationFunc"), NULL).CStr();
	//UpdateFunc = Desc.Get<CString>(CStrID("UpdateFunc"), NULL).CStr();
}
//---------------------------------------------------------------------

}