#include "TimeServer.h"

#include <Events/EventServer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MAX_FRAME_TIME 0.25f

namespace Time
{
__ImplementClassNoFactory(Time::CTimeServer, Core::CRefCounted);
__ImplementSingleton(Time::CTimeServer);

CTimeServer::CTimeServer(): _IsOpen(false), Time(0.0), FrameTime(0.0), TimeScale(1.f), LockTime(0.0), LockedFrameTime(0.0)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CTimeServer::~CTimeServer()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

void CTimeServer::Open()
{
	n_assert(!_IsOpen);
	_IsOpen = true;

	QueryPerformanceCounter((LARGE_INTEGER*)&(BasePerfTime)); // Resets time

	PrevTime = 0;
	Time = 0;
}
//---------------------------------------------------------------------

void CTimeServer::Close()
{
	TimeSources.Clear();
	Timers.Clear();

	n_assert(_IsOpen);
	_IsOpen = false;
}
//---------------------------------------------------------------------

CTime CTimeServer::GetTrueTime()
{
	LONGLONG PerfTime, PerfFreq;
	QueryPerformanceCounter((LARGE_INTEGER*)&PerfTime);
	QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFreq);
	return (CTime)((double)(PerfTime - BasePerfTime)) / ((double)PerfFreq);
}
//---------------------------------------------------------------------

void CTimeServer::AttachTimeSource(CStrID Name, PTimeSource TimeSrc)
{
	n_assert(TimeSrc.IsValid());
	n_assert(!TimeSources.Contains(Name));
	TimeSrc->Reset();
	//TimeSrc->ForceUnpause(); //???needed?
	TimeSources.Add(Name, TimeSrc);
}
//---------------------------------------------------------------------

void CTimeServer::RemoveTimeSource(CStrID Name)
{
	int Idx = TimeSources.FindIndex(Name);
	if (Idx != INVALID_INDEX) TimeSources.RemoveAt(Idx);
}
//---------------------------------------------------------------------

void CTimeServer::Save(Data::CParams& TimeParams)
{
	if (TimeSources.GetCount())
	{
		Data::PParams SGTimeSrcs = n_new(Data::CParams);
		for (int i = 0; i < TimeSources.GetCount(); ++i)
		{
			CTimeSource* pSrc = TimeSources.ValueAt(i);
			Data::PParams SGTimeSrc = n_new(Data::CParams);
			SGTimeSrc->Set(CStrID("Time"), pSrc->GetTime());
			SGTimeSrc->Set(CStrID("Factor"), pSrc->GetFactor());
			SGTimeSrc->Set(CStrID("FrameID"), (int)pSrc->GetFrameID());
			SGTimeSrc->Set(CStrID("PauseCount"), pSrc->GetPauseCount());
			SGTimeSrcs->Set(TimeSources.KeyAt(i), SGTimeSrc);
		}
		TimeParams.Set(CStrID("TimeSources"), SGTimeSrcs);
	}

	if (Timers.GetCount())
	{
		Data::PParams SGTimers = n_new(Data::CParams);
		for (int i = 0; i < Timers.GetCount(); ++i)
		{
			CTimer& Timer = Timers.ValueAt(i);
			Data::PParams SGTimer = n_new(Data::CParams);
			SGTimer->Set(CStrID("Time"), Timer.Time);
			SGTimer->Set(CStrID("CurrTime"), Timer.CurrTime);
			SGTimer->Set(CStrID("Loop"), Timer.Loop);
			SGTimer->Set(CStrID("Active"), Timer.Active);
			if (Timer.TimeSrc.IsValid())
				SGTimer->Set(CStrID("TimeSrc"), Timer.TimeSrc);
			SGTimer->Set(CStrID("EventID"), Timer.EventID);
			SGTimers->Set(Timers.KeyAt(i), SGTimer);
		}
		TimeParams.Set(CStrID("Timers"), SGTimers);
	}
}
//---------------------------------------------------------------------

void CTimeServer::Load(const Data::CParams& TimeParams)
{
	Data::PParams SubSection;
	if (TimeParams.Get<Data::PParams>(SubSection, CStrID("TimeSources")) && SubSection->GetCount())
	{
		for (int i = 0; i < SubSection->GetCount(); ++i)
		{
			const Data::CParam& Prm = SubSection->Get(i);
			Data::PParams TimeSrcDesc = Prm.GetValue<Data::PParams>();
			PTimeSource TimeSrc = TimeSources.GetOrAdd(Prm.GetName());
			TimeSrc->FrameID = TimeSrcDesc->Get<int>(CStrID("FrameID"));
			TimeSrcDesc->Get(TimeSrc->Time, CStrID("Time"));
			TimeSrcDesc->Get(TimeSrc->TimeFactor, CStrID("TimeFactor"));
			TimeSrcDesc->Get(TimeSrc->PauseCounter, CStrID("PauseCounter"));
		}
	}

	Timers.Clear();
	if (TimeParams.Get<Data::PParams>(SubSection, CStrID("Timers")) && SubSection->GetCount())
	{
		for (int i = 0; i < SubSection->GetCount(); ++i)
		{
			const Data::CParam& Prm = SubSection->Get(i);
			Data::PParams TimerDesc = Prm.GetValue<Data::PParams>();
			CTimer& Timer = Timers.Add(Prm.GetName());
			TimerDesc->Get(Timer.Time, CStrID("Time"));
			TimerDesc->Get(Timer.CurrTime, CStrID("CurrTime"));
			TimerDesc->Get(Timer.Loop, CStrID("Loop"));
			TimerDesc->Get(Timer.Active, CStrID("Active"));
			Timer.TimeSrc = TimerDesc->Get(CStrID("TimeSrc"), CStrID::Empty);
			Timer.EventID = TimerDesc->Get<CStrID>(CStrID("EventID"));
		}
	}
}
//---------------------------------------------------------------------

bool CTimeServer::CreateNamedTimer(CStrID Name, float Time, bool Loop, CStrID Event, CStrID TimeSrc)
{
	int Idx = Timers.FindIndex(Name);
	if (Idx == INVALID_INDEX)
	{
		CTimer New;
		New.TimeSrc = TimeSrc;
		New.Time = Time;
		New.EventID = Event;
		New.Loop = Loop;
		New.Active = true;
		New.CurrTime = 0.f;
		Timers.Add(Name, New);
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

// Update all time sources. This method is called very early in the frame
// by the current application state handler.
void CTimeServer::Trigger()
{
	if (LockedFrameTime > 0.0) LockTime += LockedFrameTime;

	CTime CurrTime = (LockedFrameTime > 0.0) ? LockTime : GetTrueTime();

	FrameTime = CurrTime - PrevTime;
	if (FrameTime < 0.0) FrameTime = 0.0001;
	else if (FrameTime > MAX_FRAME_TIME) FrameTime = MAX_FRAME_TIME;
	FrameTime *= TimeScale;

	PrevTime = CurrTime;
	Time += FrameTime;

	for (int i = 0; i < TimeSources.GetCount(); i++)
		TimeSources.ValueAt(i)->Update((float)FrameTime);

	for (int i = 0; i < Timers.GetCount(); i++)
	{
		CTimer& Timer = Timers.ValueAt(i);
		if (Timer.Active && !IsPaused(Timer.TimeSrc))
		{
			Timer.CurrTime += (float)GetFrameTime(Timer.TimeSrc);
			while (Timer.CurrTime >= Timer.Time)
			{
				//!!!???pre-create params once per timer?!
				Data::PParams P = n_new(Data::CParams);
				P->Set(CStrID("Name"), CString(Timers.KeyAt(i).CStr()));
				EventSrv->FireEvent(Timer.EventID, P);
				if (Timer.Loop) Timer.CurrTime -= Timer.Time;
				else
				{
					Timers.RemoveAt(i--);
					break;
				}
			}
		}
	}
}
//---------------------------------------------------------------------

void CTimeServer::LockFrameRate(CTime DesiredFrameTime)
{
	n_assert(DesiredFrameTime >= 0.0);
	if (DesiredFrameTime == 0.0)
	{
		//SetTime(LockTime);
		LONGLONG PerfFreq;
		QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFreq);
		LONGLONG PerfTimeToSet = (LONGLONG)(LockTime * ((double)PerfFreq));
		QueryPerformanceCounter((LARGE_INTEGER*)&(BasePerfTime));
		BasePerfTime -= PerfTimeToSet;
	}
	else LockTime = GetTrueTime();
	LockedFrameTime = DesiredFrameTime;
}
//---------------------------------------------------------------------

}
