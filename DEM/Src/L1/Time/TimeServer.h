#pragma once
#ifndef __DEM_L1_TIME_SERVER_H__
#define __DEM_L1_TIME_SERVER_H__

#include <Time/TimeSource.h>
#include <Core/Singleton.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <Data/Dictionary.h>

// Manages the main application timer, time sources used by different subsystems, and named timers

namespace Data
{
	class CParams;
}

namespace Time
{
#define TimeSrv Time::CTimeServer::Instance()

class CTimeServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CTimeServer);

protected:

	struct CTimer
	{
		float	Time;
		float	CurrTime;
		bool	Loop;
		bool	Active;
		CStrID	TimeSrc;
		CStrID	EventID; //???!!!CEventID?
	};

	bool								_IsOpen;
	__int64								BasePerfTime;
	CTime								PrevTime;
	CTime								Time;
	CTime								FrameTime;
	CTime								LockedFrameTime;
	CTime								LockTime;
	float								TimeScale;
	CDict<CStrID, PTimeSource>	TimeSources;
	CDict<CStrID, CTimer>			Timers;

public:

	CTimeServer();
	virtual ~CTimeServer();

	void			Open();
	void			Close();
	void			Trigger();
	void			Save(Data::CParams& TimeParams);
	void			Load(const Data::CParams& TimeParams);

	bool			IsOpen() const { return _IsOpen; }

	void			ResetAll();
	void			PauseAll();
	void			UnpauseAll();

	void			AttachTimeSource(CStrID Name, PTimeSource TimeSrc);
	void			RemoveTimeSource(CStrID Name);
	CTimeSource*	GetTimeSource(CStrID Name) const;
	CTime			GetTime(CStrID SrcName = CStrID::Empty) const;
	CTime			GetTrueTime();
	CTime			GetFrameTime(CStrID SrcName = CStrID::Empty) const;
	void			SetTimeScale(float SpeedScale) { n_assert(SpeedScale >= 0.f); TimeScale = SpeedScale; }
	void			LockFrameRate(CTime DesiredFrameTime);
	void			UnlockFrameRate() { LockFrameRate(0.0); }
	bool			IsPaused(CStrID SrcName = CStrID::Empty) const;

	bool			CreateNamedTimer(CStrID Name, float Time, bool Loop = false,
									 CStrID Event = CStrID("OCTimer"), CStrID TimeSrc = CStrID::Empty);
	void			PauseNamedTimer(CStrID Name, bool Pause = true);
	void			DestroyNamedTimer(CStrID Name);
};

// This is usually called at the beginning of an application state.
inline void CTimeServer::ResetAll()
{
	for (int i = 0; i < TimeSources.GetCount(); i++)
		TimeSources.ValueAt(i)->Reset();
}
//---------------------------------------------------------------------

// Pause all time sources. NOTE: there's an independent pause counter inside each
// time source, a pause just increments the counter, a continue decrements
// it, when the pause counter is != 0 it means, pause is activated.
inline void CTimeServer::PauseAll()
{
	for (int i = 0; i < TimeSources.GetCount(); i++)
		TimeSources.ValueAt(i)->Pause();
}
//---------------------------------------------------------------------

// This is usually called at the beginning of an application state.
inline void CTimeServer::UnpauseAll()
{
	for (int i = 0; i < TimeSources.GetCount(); i++)
		TimeSources.ValueAt(i)->Unpause();
}
//---------------------------------------------------------------------

inline CTimeSource* CTimeServer::GetTimeSource(CStrID Name) const
{
	int Idx = TimeSources.FindIndex(Name);
	return (Idx != INVALID_INDEX) ? TimeSources.ValueAt(Idx).GetUnsafe() : NULL;
}
//---------------------------------------------------------------------

inline CTime CTimeServer::GetTime(CStrID SrcName) const
{
	return (SrcName.IsValid()) ? TimeSources[SrcName]->GetTime() : Time;
}
//---------------------------------------------------------------------

inline CTime CTimeServer::GetFrameTime(CStrID SrcName) const
{
	return (SrcName.IsValid()) ? TimeSources[SrcName]->GetFrameTime() : FrameTime;
}
//---------------------------------------------------------------------

inline bool CTimeServer::IsPaused(CStrID SrcName) const
{
	return (SrcName.IsValid()) ? TimeSources[SrcName]->IsPaused() : false;
}
//---------------------------------------------------------------------

inline void CTimeServer::PauseNamedTimer(CStrID Name, bool Pause)
{
	int Idx = Timers.FindIndex(Name);
	if (Idx != INVALID_INDEX) Timers[Name].Active = !Pause;
}
//---------------------------------------------------------------------

inline void CTimeServer::DestroyNamedTimer(CStrID Name)
{
	int Idx = Timers.FindIndex(Name);
	if (Idx != INVALID_INDEX) Timers.RemoveAt(Idx);
}
//---------------------------------------------------------------------

}

#endif
