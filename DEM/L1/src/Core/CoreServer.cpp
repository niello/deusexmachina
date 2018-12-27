#include "CoreServer.h"

#include <Core/Object.h>
#include <Core/TimeSource.h>
#include <Events/EventServer.h>

#define MAX_FRAME_TIME 0.25f

namespace Core
{
__ImplementSingleton(Core::CCoreServer);

const CString CCoreServer::Mem_HighWaterSize("Mem_HighWaterSize");
const CString CCoreServer::Mem_TotalSize("Mem_TotalSize");
const CString CCoreServer::Mem_TotalCount("Mem_TotalCount");

CCoreServer::CCoreServer():
	Time(0.0),
	PrevTime(0.0),
	FrameTime(0.0),
	TimeScale(1.f),
	LockTime(0.0),
	LockedFrameTime(0.0)
{
	__ConstructSingleton;
	BaseTime = Sys::GetAppTime();
	n_dbgmeminit();
}
//---------------------------------------------------------------------

CCoreServer::~CCoreServer()
{
	TimeSources.Clear();
	Timers.Clear();

#ifdef _DEBUG
	CObject::DumpLeaks();
#endif

	// It dumps leaks to a file!
	//if (n_dbgmemdumpleaks() != 0)
	//	n_dbgout("n_dbgmemdumpleaks detected and dumped memory leaks");

	__DestructSingleton;
}
//---------------------------------------------------------------------

void CCoreServer::Trigger()
{
	if (LockedFrameTime > 0.0) LockTime += LockedFrameTime;

	CTime CurrTime = (LockedFrameTime > 0.0) ? LockTime : Sys::GetAppTime();

	FrameTime = CurrTime - PrevTime;
	if (FrameTime < 0.0) FrameTime = 0.0001;
	else if (FrameTime > MAX_FRAME_TIME) FrameTime = MAX_FRAME_TIME;
	FrameTime *= TimeScale;

	PrevTime = CurrTime;
	Time += FrameTime;

	for (UPTR i = 0; i < TimeSources.GetCount(); ++i)
		TimeSources.ValueAt(i)->Update((float)FrameTime);

	for (UPTR i = 0; i < Timers.GetCount(); ++i)
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

#ifdef DEM_STATS
	CMemoryStats Stats = n_dbgmemgetstats();
	CoreSrv->SetGlobal<int>(Mem_HighWaterSize, Stats.HighWaterSize);
	CoreSrv->SetGlobal<int>(Mem_TotalSize, Stats.TotalSize);
	CoreSrv->SetGlobal<int>(Mem_TotalCount, Stats.TotalCount);
#endif
}
//---------------------------------------------------------------------

void CCoreServer::AttachTimeSource(CStrID Name, PTimeSource TimeSrc)
{
	n_assert(TimeSrc.IsValidPtr());
	n_assert(!TimeSources.Contains(Name));
	TimeSrc->Reset();
	//TimeSrc->ForceUnpause(); //???needed?
	TimeSources.Add(Name, TimeSrc);
}
//---------------------------------------------------------------------

void CCoreServer::RemoveTimeSource(CStrID Name)
{
	IPTR Idx = TimeSources.FindIndex(Name);
	if (Idx != INVALID_INDEX) TimeSources.RemoveAt(Idx);
}
//---------------------------------------------------------------------

void CCoreServer::Save(Data::CParams& TimeParams)
{
	if (TimeSources.GetCount())
	{
		Data::PParams SGTimeSrcs = n_new(Data::CParams);
		for (UPTR i = 0; i < TimeSources.GetCount(); ++i)
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
		for (UPTR i = 0; i < Timers.GetCount(); ++i)
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

void CCoreServer::Load(const Data::CParams& TimeParams)
{
	Data::PParams SubSection;
	if (TimeParams.Get<Data::PParams>(SubSection, CStrID("TimeSources")) && SubSection->GetCount())
	{
		for (UPTR i = 0; i < SubSection->GetCount(); ++i)
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
		for (UPTR i = 0; i < SubSection->GetCount(); ++i)
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

CTimeSource* CCoreServer::GetTimeSource(CStrID Name) const
{
	IPTR Idx = TimeSources.FindIndex(Name);
	return (Idx != INVALID_INDEX) ? TimeSources.ValueAt(Idx).GetUnsafe() : NULL;
}
//---------------------------------------------------------------------

CTime CCoreServer::GetTime(CStrID SrcName) const
{
	return (SrcName.IsValid()) ? TimeSources[SrcName]->GetTime() : Time;
}
//---------------------------------------------------------------------

CTime CCoreServer::GetFrameTime(CStrID SrcName) const
{
	return (SrcName.IsValid()) ? TimeSources[SrcName]->GetFrameTime() : FrameTime;
}
//---------------------------------------------------------------------

bool CCoreServer::IsPaused(CStrID SrcName) const
{
	return (SrcName.IsValid()) ? TimeSources[SrcName]->IsPaused() : false;
}
//---------------------------------------------------------------------

bool CCoreServer::CreateNamedTimer(CStrID Name, float Time, bool Loop, CStrID Event, CStrID TimeSrc)
{
	IPTR Idx = Timers.FindIndex(Name);
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

void CCoreServer::PauseNamedTimer(CStrID Name, bool Pause)
{
	IPTR Idx = Timers.FindIndex(Name);
	if (Idx != INVALID_INDEX) Timers[Name].Active = !Pause;
}
//---------------------------------------------------------------------

void CCoreServer::DestroyNamedTimer(CStrID Name)
{
	IPTR Idx = Timers.FindIndex(Name);
	if (Idx != INVALID_INDEX) Timers.RemoveAt(Idx);
}
//---------------------------------------------------------------------

// This is usually called at the beginning of an application state.
void CCoreServer::ResetAll()
{
	for (UPTR i = 0; i < TimeSources.GetCount(); ++i)
		TimeSources.ValueAt(i)->Reset();
}
//---------------------------------------------------------------------

// Pause all time sources. NOTE: there's an independent pause counter inside each
// time source, a pause just increments the counter, a continue decrements
// it, when the pause counter is != 0 it means, pause is activated.
void CCoreServer::PauseAll()
{
	for (UPTR i = 0; i < TimeSources.GetCount(); ++i)
		TimeSources.ValueAt(i)->Pause();
}
//---------------------------------------------------------------------

// This is usually called at the beginning of an application state.
void CCoreServer::UnpauseAll()
{
	for (UPTR i = 0; i < TimeSources.GetCount(); ++i)
		TimeSources.ValueAt(i)->Unpause();
}
//---------------------------------------------------------------------

void CCoreServer::LockFrameRate(CTime DesiredFrameTime)
{
	n_assert(DesiredFrameTime >= 0.0);
	if (DesiredFrameTime == 0.0)
		BaseTime = Sys::GetAppTime() - LockTime;
	else LockTime = Sys::GetAppTime();
	LockedFrameTime = DesiredFrameTime;
}
//---------------------------------------------------------------------

}