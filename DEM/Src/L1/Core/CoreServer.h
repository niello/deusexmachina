#pragma once
#ifndef __DEM_L1_CORE_SERVER_H__
#define __DEM_L1_CORE_SERVER_H__

#include <Core/Singleton.h>
#include <Data/Data.h>
#include <Data/HashMap.h>

// Core server manages low-level object framework

namespace Core
{
#define CoreSrv Core::CCoreServer::Instance()

class CCoreServer
{
	__DeclareSingleton(CCoreServer);

private:

	bool _IsOpen;

public:

	CHashMap<Data::CData> Globals;

	CCoreServer();
	~CCoreServer();

	bool Open();
	void Close();
	void Trigger();

	template<class T> void		SetGlobal(const nString& Name, const T& Value) { Globals.At(Name.CStr()) = Value; }
	template<class T> T&		GetGlobal(const nString& Name) { return Globals[Name.CStr()].GetValue<T>(); }
	template<class T> bool		GetGlobal(const nString& Name, T& OutValue) const;
	template<> bool				GetGlobal(const nString& Name, Data::CData& OutValue) const { return Globals.Get(Name.CStr(), OutValue); }
};

template<class T> inline bool CCoreServer::GetGlobal(const nString& Name, T& OutValue) const
{
	Data::CData Data;
	return Globals.Get(Name.CStr(), Data) ? Data.GetValue<T>(OutValue) : false;
}
//---------------------------------------------------------------------

}

#endif
