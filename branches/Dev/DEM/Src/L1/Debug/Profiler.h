#ifndef N_PROFILER_H
#define N_PROFILER_H

#include <Time/TimeServer.h>
#include <Core/CoreServer.h>

// Profiler provides an easy way to measure time intervals.
// (C) 2002 RadonLabs GmbH

class CProfiler
{
private:

	CString	GlobalVarName;
	CTime	StartTime;
	CTime	AccumTime;
	bool	IsActive;

public:

	CProfiler(): StartTime(0.0), AccumTime(0.0), IsActive(false) {}
	CProfiler(const char* pName) { Initialize(pName); }

	void Initialize(const char* pName);
	void Start();
	void Stop();
	void ResetAccum() { AccumTime = 0.0; }
	void StartAccum() { Start(); }
	void StopAccum();

	CTime	GetTime() { return (TimeSrv->GetTrueTime() - StartTime) * 1000.0f; }
	bool	IsValid() const { return GlobalVarName.IsValid(); }
	bool	IsStarted() const { return IsActive; }
};

#ifdef DEM_STATS
#define PROFILER_DECLARE(Prof)		CProfiler Prof;
#define PROFILER_INIT(Prof, Name)	Prof.Initialize(Name);
#define PROFILER_START(Prof)		Prof.Start();
#define PROFILER_STOP(Prof)			Prof.Stop();
#define PROFILER_RESET(Prof)		Prof.ResetAccum();
#define PROFILER_STARTACCUM(Prof)	Prof.StartAccum();
#define PROFILER_STOPACCUM(Prof)	Prof.StopAccum();
#else
#define PROFILER_DECLARE(Prof)
#define PROFILER_INIT(Prof, Name)
#define PROFILER_START(Prof)
#define PROFILER_STOP(Prof)
#define PROFILER_RESET(Prof)
#define PROFILER_STARTACCUM(Prof)
#define PROFILER_STOPACCUM(Prof)
#endif

inline void CProfiler::Initialize(const char* pName)
{
	n_assert(pName);
	GlobalVarName = pName;
	StartTime = 0.0;
	AccumTime = 0.0;
	IsActive = false;
}
//---------------------------------------------------------------------

inline void CProfiler::Start()
{
	if (IsActive) Stop();
	StartTime = TimeSrv->GetTrueTime();
	IsActive = true;
}
//---------------------------------------------------------------------

inline void CProfiler::Stop()
{
	if (!IsActive) return;
	CoreSrv->SetGlobal(GlobalVarName, (float)GetTime());
	IsActive = false;
}
//---------------------------------------------------------------------

inline void CProfiler::StopAccum()
{
	n_assert(IsActive);
	AccumTime += TimeSrv->GetTrueTime() - StartTime;
	CoreSrv->SetGlobal(GlobalVarName, (float)AccumTime * 1000.0f);
	IsActive = false;
}
//---------------------------------------------------------------------

#endif
