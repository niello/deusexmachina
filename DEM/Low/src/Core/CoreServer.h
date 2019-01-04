#pragma once
#ifndef __DEM_L1_CORE_SERVER_H__
#define __DEM_L1_CORE_SERVER_H__

#include <Data/Singleton.h>
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/Params.h>
#include <Data/HashTable.h>
#include <Data/Dictionary.h>

// Core server manages low-level object framework, memory stats and timing

//???!!!move to application base class?! too little functionality and will not be more
//isn't worth a dedicated singleton

namespace Core
{
#define CoreSrv Core::CCoreServer::Instance()

typedef Ptr<class CTimeSource> PTimeSource;

class CCoreServer
{
	__DeclareSingleton(CCoreServer);

protected:

	static const CString Mem_HighWaterSize;
	static const CString Mem_TotalSize;
	static const CString Mem_TotalCount;

	struct CTimer
	{
		float	Time;
		float	CurrTime;
		bool	Loop;
		bool	Active;
		CStrID	TimeSrc;
		CStrID	EventID;
	};

	CTime						BaseTime;
	CTime						PrevTime;
	CTime						Time;
	CTime						FrameTime;
	CTime						LockedFrameTime;
	CTime						LockTime;
	float						TimeScale;
	CDict<CStrID, PTimeSource>	TimeSources;
	CDict<CStrID, CTimer>		Timers;

public:

	CHashTable<CString, Data::CData> Globals; // Left public for iteration

	CCoreServer();
	~CCoreServer();

	void			Trigger();
	void			Save(Data::CParams& TimeParams);
	void			Load(const Data::CParams& TimeParams);

	void			ResetAll();
	void			PauseAll();
	void			UnpauseAll();

	void			AttachTimeSource(CStrID Name, PTimeSource TimeSrc);
	void			RemoveTimeSource(CStrID Name);
	CTimeSource*	GetTimeSource(CStrID Name) const;
	CTime			GetTime(CStrID SrcName = CStrID::Empty) const;
	CTime			GetFrameTime(CStrID SrcName = CStrID::Empty) const;
	void			SetTimeScale(float SpeedScale) { n_assert(SpeedScale >= 0.f); TimeScale = SpeedScale; }
	void			LockFrameRate(CTime DesiredFrameTime);
	void			UnlockFrameRate() { LockFrameRate(0.0); }
	bool			IsPaused(CStrID SrcName = CStrID::Empty) const;

	bool			CreateNamedTimer(CStrID Name, float Time, bool Loop = false,
									 CStrID Event = CStrID("OnTimer"), CStrID TimeSrc = CStrID::Empty);
	void			PauseNamedTimer(CStrID Name, bool Pause = true);
	void			DestroyNamedTimer(CStrID Name);

	template<class T> void	SetGlobal(const CString& Name, const T& Value) { Globals.At(Name) = Value; }
	template<class T> T&	GetGlobal(const CString& Name) { return Globals[Name].GetValue<T>(); }
	template<class T> bool	GetGlobal(const CString& Name, T& OutValue) const;
	template<> bool			GetGlobal(const CString& Name, Data::CData& OutValue) const { return Globals.Get(Name, OutValue); }
};

template<class T> inline bool CCoreServer::GetGlobal(const CString& Name, T& OutValue) const
{
	Data::CData Data;
	return Globals.Get(Name, Data) ? Data.GetValue<T>(OutValue) : false;
}
//---------------------------------------------------------------------

}

#endif
