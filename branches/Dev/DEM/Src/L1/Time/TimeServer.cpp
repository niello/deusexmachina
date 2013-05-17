#include "TimeServer.h"

#include <Events/EventManager.h>
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

	// ResetTime
	QueryPerformanceCounter((LARGE_INTEGER*)&(BasePerfTime));

	PrevTime = 0;
	Time = 0;

	SUBSCRIBE_PEVENT(OnLoad, CTimeServer, OnLoad);
	SUBSCRIBE_PEVENT(OnSave, CTimeServer, OnSave);
}
//---------------------------------------------------------------------

void CTimeServer::Close()
{
	UNSUBSCRIBE_EVENT(OnLoad);
	UNSUBSCRIBE_EVENT(OnSave);

	TimeSources.Clear();

	n_assert(_IsOpen);
	_IsOpen = false;
}
//---------------------------------------------------------------------

nTime CTimeServer::GetTrueTime()
{
	LONGLONG PerfTime, PerfFreq;
	QueryPerformanceCounter((LARGE_INTEGER*)&PerfTime);
	QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFreq);
	return (nTime)((double)(PerfTime - BasePerfTime)) / ((double)PerfFreq);
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
	if (Idx != INVALID_INDEX) TimeSources.EraseAt(Idx);
}
//---------------------------------------------------------------------

// Checks whether the TimeSources table exists in the database, if yes invokes OnLoad() on all time sources
bool CTimeServer::OnLoad(const Events::CEventBase& Event)
{
	/*
#ifndef _EDITOR
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));

	int Idx = pDB->FindTableIndex("TimeSources");
	if (Idx != INVALID_INDEX)
	{
		DB::PDataset DS = pDB->GetTable(Idx)->CreateDataset();
		DS->AddColumnsFromTable();
		DS->PerformQuery();
		for (int i = 0; i < DS->GetValueTable()->GetRowCount(); i++)
		{
			DS->SetRowIndex(i);
			CTimeSource* Src = GetTimeSource(CStrID(DS->Get<nString>(CStrID("TimeSourceID")).CStr()));
		}
	}
	else ResetAll(); //!!!check if it is needed! (added by me, see state handlers)	
#endif
*/

	OK;
}
//---------------------------------------------------------------------

// Ask all time sources to save their status to the database.
bool CTimeServer::OnSave(const Events::CEventBase& Event)
{
/*
#ifndef _EDITOR
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));

	DB::PTable Tbl;
	int Idx = pDB->FindTableIndex("TimeSources");
	if (Idx == INVALID_INDEX)
	{
		Tbl = DB::CTable::Create();
		Tbl->SetName("TimeSources");
		Tbl->AddColumn(DB::CColumn(Attr::TimeSourceID, DB::CColumn::Primary));
		Tbl->AddColumn(Attr::TimeSourceTime);
		Tbl->AddColumn(Attr::TimeSourceFactor);
		Tbl->AddColumn(Attr::TimeSourceFrameID);
		pDB->AddTable(Tbl);
	}
	else Tbl = pDB->GetTable(Idx);

	DB::PDataset DS = Tbl->CreateDataset();
	DS->AddColumnsFromTable();
	//for (int i = 0; i < TimeSources.GetCount(); i++)
	//	TimeSources.ValueAtIndex(i)->OnSave(DS);
	//pVT->Set<nString>(Attr::TimeSourceID, GetClassName());
	//pVT->Set<float>(Attr::TimeSourceTime, float(Time));
	//pVT->Set<float>(Attr::TimeSourceFactor, TimeFactor);
	//pVT->Set<int>(Attr::TimeSourceFrameID, FrameId);
	DS->CommitChanges();
#endif
*/

	OK;
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

	nTime CurrTime = (LockedFrameTime > 0.0) ? LockTime : GetTrueTime();

	FrameTime = CurrTime - PrevTime;
	if (FrameTime < 0.0) FrameTime = 0.0001;
	else if (FrameTime > MAX_FRAME_TIME) FrameTime = MAX_FRAME_TIME;
	FrameTime *= TimeScale;

	PrevTime = CurrTime;
	Time += FrameTime;

	for (int i = 0; i < TimeSources.GetCount(); i++)
		TimeSources.ValueAtIndex(i)->Update(FrameTime);

	for (int i = 0; i < Timers.GetCount(); i++)
	{
		CTimer& Timer = Timers.ValueAtIndex(i);
		if (Timer.Active && !IsPaused(Timer.TimeSrc))
		{
			Timer.CurrTime += (float)GetFrameTime(Timer.TimeSrc);
			while (Timer.CurrTime >= Timer.Time)
			{
				//!!!???pre-create params once per timer?!
				Data::PParams P = n_new(Data::CParams);
				P->Set(CStrID("Name"), nString(Timers.KeyAtIndex(i).CStr()));
				EventMgr->FireEvent(Timer.EventID, P);
				if (Timer.Loop) Timer.CurrTime -= Timer.Time;
				else
				{
					Timers.EraseAt(i--);
					break;
				}
			}
		}
	}
}
//---------------------------------------------------------------------

void CTimeServer::LockFrameRate(nTime DesiredFrameTime)
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

/*
bool stopped;
LONGLONG time_stop;

void ResetTime()
{
	LockTime = 0.0;
	QueryPerformanceCounter((LARGE_INTEGER*)&(BasePerfTime));
}

void
nTimeServer::SetTime(double t)
{
    LockTime = t;

    LONGLONG PerfFreq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFreq);
    LONGLONG td = (LONGLONG)(t * ((double)PerfFreq));
    QueryPerformanceCounter((LARGE_INTEGER*)&(BasePerfTime));
    time_stop = BasePerfTime;
    BasePerfTime -= td;
}

void
nTimeServer::StopTime()
{
    if (!stopped)
    {
        stopped = true;
        QueryPerformanceCounter((LARGE_INTEGER*)&(time_stop));
    }
}

void
nTimeServer::StartTime()
{
    if (stopped)
    {
        stopped = false;
        LONGLONG PerfTime, td;
        QueryPerformanceCounter((LARGE_INTEGER*)&PerfTime);
        td = PerfTime - time_stop;
        BasePerfTime += td;
    }
}
*/