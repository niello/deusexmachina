#ifndef N_PROFILER_H
#define N_PROFILER_H
//------------------------------------------------------------------------------
/**
    @class nProfiler
    @ingroup Time
    @brief nProfiler provides an easy way to measure time intervals.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/nkernelserver.h"
#include "kernel/nref.h"
#include <Time/TimeServer.h>

//------------------------------------------------------------------------------
class nProfiler
{
private:

	nString		GlobalVarName;
	nTime		startTime;
	bool		isStarted;
	nTime		accumTime;

public:

	nProfiler(): startTime(0.0), isStarted(false), accumTime(0.0) {}
	nProfiler(const char* name) { Initialize(name); }

	void Initialize(const char* name);
	bool IsValid() const { return GlobalVarName.IsValid(); }
	void Start();
	bool IsStarted() const { return isStarted; }
	void Stop();
	void ResetAccum() { accumTime = 0.0; }
	void StartAccum() { Start(); }
	void StopAccum();
	nTime GetTime() { return (TimeSrv->GetTrueTime() - startTime) * 1000.0f; }
};

#if __NEBULA_STATS__
#define PROFILER_DECLARE(prof) nProfiler prof;
#define PROFILER_INIT(prof,name) prof.Initialize(name);
#define PROFILER_START(prof) prof.Start();
#define PROFILER_STOP(prof)  prof.Stop();
#define PROFILER_RESET(prof) prof.ResetAccum();
#define PROFILER_STARTACCUM(prof) prof.StartAccum();
#define PROFILER_STOPACCUM(prof)  prof.StopAccum();
#else
#define PROFILER_DECLARE(prof)
#define PROFILER_INIT(prof,name)
#define PROFILER_START(prof)
#define PROFILER_STOP(prof)
#define PROFILER_RESET(prof)
#define PROFILER_STARTACCUM(prof)
#define PROFILER_STOPACCUM(prof)
#endif

inline void nProfiler::Initialize(const char* name)
{
	n_assert(name);
	char buf[N_MAXPATH];
	snprintf(buf, sizeof(buf), "/sys/var/%s", name);
	startTime = 0.0;
	isStarted = false;
	accumTime = 0.0;
}
//---------------------------------------------------------------------

inline void nProfiler::Start()
{
	if (isStarted) Stop();
	startTime = TimeSrv->GetTrueTime();
	isStarted = true;
}
//---------------------------------------------------------------------

inline void nProfiler::Stop()
{
	if (!isStarted) return;
	CoreSrv->SetGlobal(GlobalVarName, (float)GetTime());
	isStarted = false;
}
//---------------------------------------------------------------------

inline void nProfiler::StopAccum()
{
	n_assert(isStarted);
	accumTime += TimeSrv->GetTrueTime() - startTime;
	CoreSrv->SetGlobal(GlobalVarName, (float)accumTime * 1000.0f);
	isStarted = false;
}
//---------------------------------------------------------------------

#endif
