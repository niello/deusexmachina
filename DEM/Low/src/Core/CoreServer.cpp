#include "CoreServer.h"

#include <Core/Object.h>
#include <Core/TimeSource.h>
#include <Events/EventServer.h>

#define MAX_FRAME_TIME 0.25f

namespace Core
{
__ImplementSingleton(Core::CCoreServer);

const std::string CCoreServer::Mem_HighWaterSize("Mem_HighWaterSize");
const std::string CCoreServer::Mem_TotalSize("Mem_TotalSize");
const std::string CCoreServer::Mem_TotalCount("Mem_TotalCount");

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
	TimeSources.clear();
	Timers.clear();

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

	for (auto& [Name, TimeSource] : TimeSources)
		TimeSource->Update((float)FrameTime);

	for (auto It = Timers.begin(); It != Timers.end(); /**/)
	{
		auto& Timer = It->second;
		bool Erase = false;
		if (Timer.Active && !IsPaused(Timer.TimeSrc))
		{
			Timer.CurrTime += (float)GetFrameTime(Timer.TimeSrc);
			while (Timer.CurrTime >= Timer.Time)
			{
				//!!!???pre-create params once per timer?!
				Data::PParams P = n_new(Data::CParams);
				P->Set(CStrID("Name"), CString(It->first.CStr()));
				EventSrv->FireEvent(Timer.EventID, P);
				if (Timer.Loop) Timer.CurrTime -= Timer.Time;
				else
				{
					Erase = true;
					break;
				}
			}
		}

		if (Erase) It = Timers.erase(It);
		else ++It;
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
	n_assert(TimeSources.find(Name) == TimeSources.cend());
	TimeSrc->Reset();
	//TimeSrc->ForceUnpause(); //???needed?
	TimeSources.emplace(Name, TimeSrc);
}
//---------------------------------------------------------------------

void CCoreServer::RemoveTimeSource(CStrID Name)
{
	TimeSources.erase(Name);
}
//---------------------------------------------------------------------

void CCoreServer::Save(Data::CParams& TimeParams)
{
	if (TimeSources.size())
	{
		Data::PParams SGTimeSrcs = n_new(Data::CParams);
		for (auto& [Key, Src] : TimeSources)
		{
			Data::PParams SGTimeSrc = n_new(Data::CParams);
			SGTimeSrc->Set(CStrID("Time"), Src->GetTime());
			SGTimeSrc->Set(CStrID("Factor"), Src->GetFactor());
			SGTimeSrc->Set(CStrID("FrameID"), (int)Src->GetFrameID());
			SGTimeSrc->Set(CStrID("PauseCount"), Src->GetPauseCount());
			SGTimeSrcs->Set(Key, SGTimeSrc);
		}
		TimeParams.Set(CStrID("TimeSources"), SGTimeSrcs);
	}

	if (Timers.size())
	{
		Data::PParams SGTimers = n_new(Data::CParams);
		for (auto& [Key, Timer] : Timers)
		{
			Data::PParams SGTimer = n_new(Data::CParams);
			SGTimer->Set(CStrID("Time"), Timer.Time);
			SGTimer->Set(CStrID("CurrTime"), Timer.CurrTime);
			SGTimer->Set(CStrID("Loop"), Timer.Loop);
			SGTimer->Set(CStrID("Active"), Timer.Active);
			if (Timer.TimeSrc.IsValid())
				SGTimer->Set(CStrID("TimeSrc"), Timer.TimeSrc);
			SGTimer->Set(CStrID("EventID"), Timer.EventID);
			SGTimers->Set(Key, SGTimer);
		}
		TimeParams.Set(CStrID("Timers"), SGTimers);
	}
}
//---------------------------------------------------------------------

void CCoreServer::Load(const Data::CParams& TimeParams)
{
	Data::PParams SubSection;
	if (TimeParams.TryGet<Data::PParams>(SubSection, CStrID("TimeSources")) && SubSection->GetCount())
	{
		for (UPTR i = 0; i < SubSection->GetCount(); ++i)
		{
			const Data::CParam& Prm = SubSection->Get(i);
			Data::PParams TimeSrcDesc = Prm.GetValue<Data::PParams>();
			PTimeSource TimeSrc = TimeSources[Prm.GetName()];
			TimeSrc->FrameID = TimeSrcDesc->Get<int>(CStrID("FrameID"));
			TimeSrcDesc->TryGet(TimeSrc->Time, CStrID("Time"));
			TimeSrcDesc->TryGet(TimeSrc->TimeFactor, CStrID("TimeFactor"));
			TimeSrcDesc->TryGet(TimeSrc->PauseCounter, CStrID("PauseCounter"));
		}
	}

	Timers.clear();
	if (TimeParams.TryGet<Data::PParams>(SubSection, CStrID("Timers")) && SubSection->GetCount())
	{
		for (UPTR i = 0; i < SubSection->GetCount(); ++i)
		{
			const Data::CParam& Prm = SubSection->Get(i);
			Data::PParams TimerDesc = Prm.GetValue<Data::PParams>();
			CTimer& Timer = Timers[Prm.GetName()];
			TimerDesc->TryGet(Timer.Time, CStrID("Time"));
			TimerDesc->TryGet(Timer.CurrTime, CStrID("CurrTime"));
			TimerDesc->TryGet(Timer.Loop, CStrID("Loop"));
			TimerDesc->TryGet(Timer.Active, CStrID("Active"));
			Timer.TimeSrc = TimerDesc->Get(CStrID("TimeSrc"), CStrID::Empty);
			Timer.EventID = TimerDesc->Get<CStrID>(CStrID("EventID"));
		}
	}
}
//---------------------------------------------------------------------

CTimeSource* CCoreServer::GetTimeSource(CStrID Name) const
{
	auto It = TimeSources.find(Name);
	return (It != TimeSources.cend()) ? It->second.Get() : nullptr;
}
//---------------------------------------------------------------------

CTime CCoreServer::GetTime(CStrID SrcName) const
{
	if (!SrcName) return Time;

	auto It = TimeSources.find(SrcName);
	return (It != TimeSources.cend()) ? It->second->GetTime() : 0;
}
//---------------------------------------------------------------------

CTime CCoreServer::GetFrameTime(CStrID SrcName) const
{
	if (!SrcName) return FrameTime;

	auto It = TimeSources.find(SrcName);
	return (It != TimeSources.cend()) ? It->second->GetFrameTime() : 0;
}
//---------------------------------------------------------------------

bool CCoreServer::IsPaused(CStrID SrcName) const
{
	if (!SrcName) return false;

	auto It = TimeSources.find(SrcName);
	return (It != TimeSources.cend()) ? It->second->IsPaused() : false;
}
//---------------------------------------------------------------------

bool CCoreServer::CreateNamedTimer(CStrID Name, float Time, bool Loop, CStrID Event, CStrID TimeSrc)
{
	auto It = Timers.find(Name);
	if (It == Timers.cend())
	{
		CTimer New;
		New.TimeSrc = TimeSrc;
		New.Time = Time;
		New.EventID = Event;
		New.Loop = Loop;
		New.Active = true;
		New.CurrTime = 0.f;
		Timers.emplace(Name, std::move(New));
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

void CCoreServer::PauseNamedTimer(CStrID Name, bool Pause)
{
	auto It = Timers.find(Name);
	if (It != Timers.cend())
		It->second.Active = !Pause;
}
//---------------------------------------------------------------------

void CCoreServer::DestroyNamedTimer(CStrID Name)
{
	Timers.erase(Name);
}
//---------------------------------------------------------------------

// This is usually called at the beginning of an application state.
void CCoreServer::ResetAll()
{
	for (auto& [Key, Src] : TimeSources)
		Src->Reset();
}
//---------------------------------------------------------------------

// Pause all time sources. NOTE: there's an independent pause counter inside each
// time source, a pause just increments the counter, a continue decrements
// it, when the pause counter is != 0 it means, pause is activated.
void CCoreServer::PauseAll()
{
	for (auto& [Key, Src] : TimeSources)
		Src->Pause();
}
//---------------------------------------------------------------------

// This is usually called at the beginning of an application state.
void CCoreServer::UnpauseAll()
{
	for (auto& [Key, Src] : TimeSources)
		Src->Unpause();
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
