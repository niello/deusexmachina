#pragma once
#ifndef __DEM_L1_CORE_SERVER_H__
#define __DEM_L1_CORE_SERVER_H__

#include <Data/Singleton.h>
#include <Data/Data.h>
#include <util/nlist.h>
#include <util/HashMap.h>

// Core server manages low-level object framework

class nKernelServer;

namespace Core
{
#define CoreSrv Core::CCoreServer::Instance()

class CCoreServer
{
	__DeclareSingleton(CCoreServer);

private:

	friend class CRefCounted;

	bool					_IsOpen;
	static nList			RefCountedList;
	CHashMap<Data::CData>	Globals; //!!!Need iterability!

public:

	CCoreServer();
	~CCoreServer();

	bool Open();
	void Close();

	template<class T> void		SetGlobal(const nString& Name, const T& Value) { Globals.At(Name.Get()) = Value; }
	template<class T> T&		GetGlobal(const nString& Name) { return Globals[Name.Get()].GetValue<T>(); }
	template<class T> bool		GetGlobal(const nString& Name, T& OutValue) const;
	bool						GetGlobal(const nString& Name, Data::CData& OutValue) const { return Globals.Get(Name.Get(), OutValue); }
};

template<class T> inline bool CCoreServer::GetGlobal(const nString& Name, T& OutValue) const
{
	Data::CData Data;
	return Globals.Get(Name.Get(), Data) ? Data.GetValue<T>(OutValue) : false;
}
//---------------------------------------------------------------------

}

#endif
