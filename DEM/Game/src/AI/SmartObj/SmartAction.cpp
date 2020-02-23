#include "SmartAction.h"

#include <AI/Planning/WorldStateSource.h>
#include <Scripting/ScriptObject.h>
#include <Core/Factory.h>

namespace AI
{
CSmartAction::CSmartAction() {}
CSmartAction::~CSmartAction() {}

void CSmartAction::Init(CStrID ActionID, const Data::CParams& Desc)
{
	n_assert(ActionID.IsValid());

	ID = ActionID;

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

	CString PD = Desc.Get<CString>(CStrID("ProgressDriver"), CString::Empty);
	PD.Trim();
	PD.ToLower();
	if (PD == "dur" || PD == "duration") ProgressDriver = PDrv_Duration;
	else if (PD == "fsm" || PD == "so.fsm") ProgressDriver = PDrv_SO_FSM;
	else ProgressDriver = PDrv_None;

	MaxUserCount = Desc.Get<int>(CStrID("MaxUserCount"), -1);

	static const CString StrWSSrcPfx("AI::CWorldStateSource");
	Data::PParams SubDesc;
	if (Desc.TryGet<Data::PParams>(SubDesc, CStrID("Preconditions")))
	{
		Preconditions = Core::CFactory::Instance().Create<CWorldStateSource>(StrWSSrcPfx + SubDesc->Get<CString>(CStrID("Type")));
		Preconditions->Init(SubDesc);
	}
	else Preconditions = nullptr;

	const CString& ScriptName = Desc.Get<CString>(CStrID("Script"), CString::Empty);
	if (ScriptName.IsValid())
	{
		ScriptObj = n_new(Scripting::CScriptObject(ID.CStr(), "Actions"));
		ScriptObj->Init(); // No special class
		CString ScriptFile = "Scripts:" + ScriptName + ".lua";
		if (ScriptObj->LoadScriptFile(ScriptFile) != Success)
			Sys::Log("Error loading script \"%s\" for an SO action", ScriptFile.CStr());
	}
}
//---------------------------------------------------------------------

bool CSmartAction::IsValid(CStrID ActorID, CStrID SOID) const
{
	if (ScriptObj.IsNullPtr()) OK;
	Data::CData Args[] = { ActorID, SOID };
	UPTR Res = ScriptObj->RunFunction("IsValid", Args, 2);
	return Res == Success || Res == Error_Scripting_NoFunction;
}
//---------------------------------------------------------------------

float CSmartAction::GetDuration(CStrID ActorID, CStrID SOID) const
{
	if (ScriptObj.IsValidPtr())
	{
		Data::CData Args[] = { ActorID, SOID };
		Data::CData RetVal;
		UPTR Res = ScriptObj->RunFunction("GetDuration", Args, 2, &RetVal);
		if (Res != Error_Scripting_NoFunction)
		{
			//!!!need conversion!
			if (RetVal.IsA<float>()) return RetVal.GetValue<float>();
			else if (RetVal.IsA<int>()) return (float)RetVal.GetValue<int>();
		}
	}

	return Duration;
}
//---------------------------------------------------------------------

UPTR CSmartAction::Update(CStrID ActorID, CStrID SOID) const
{
	if (ScriptObj.IsValidPtr())
	{
		Data::CData Args[] = { ActorID, SOID };
		Data::CData RetVal;
		UPTR Res = ScriptObj->RunFunction("Update", Args, 2, &RetVal);
		if (Res != Error_Scripting_NoFunction)
			return RetVal.GetValue<int>();
	}

	return Running;
}
//---------------------------------------------------------------------

}