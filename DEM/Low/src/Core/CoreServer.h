#pragma once
#include <Data/Singleton.h>
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/Params.h>
#include <map>

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

	static const std::string Mem_HighWaterSize;
	static const std::string Mem_TotalSize;
	static const std::string Mem_TotalCount;

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
	std::map<CStrID, PTimeSource> TimeSources;
	std::map<CStrID, CTimer>	Timers;

public:

	std::unordered_map<std::string, Data::CData> Globals; // Left public for iteration

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

	template<class T> void	SetGlobal(const std::string& Name, const T& Value) { Globals[Name] = Value; }
	template<class T> T&	GetGlobal(const std::string& Name) { return Globals[Name].GetValue<T>(); }

	template<class T> bool GetGlobal(const std::string& Name, T& OutValue) const
	{
		auto It = Globals.find(Name);
		if (It == Globals.cend()) return false;
		It->second.GetValue<T>(OutValue);
		return true;
	}

	template<> bool GetGlobal(const std::string& Name, Data::CData& OutValue) const
	{
		auto It = Globals.find(Name);
		if (It == Globals.cend()) return false;
		OutValue = It->second;
		return true;
	}
};

}
