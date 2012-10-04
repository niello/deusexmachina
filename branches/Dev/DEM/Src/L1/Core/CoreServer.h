#pragma once
#ifndef __DEM_L1_CORE_SERVER_H__
#define __DEM_L1_CORE_SERVER_H__

#include <Data/Singleton.h>
#include <Data/Data.h>
#include <util/nlist.h>
#include <util/nhashmap2.h>

// The Core subsystem establishes a basic runtime environment, and provides pointers to
// low level objects (mainly Nebula servers) to whomever wants them.

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
	nKernelServer*			pKernelServer;
	static nList			RefCountedList;
	nHashMap2<Data::CData>	Globals; //!!!Need iterability!

public:

	CCoreServer(const nString& vendor, const nString& app);
	~CCoreServer();

	bool Open();
	void Close();

	template<class T> void		SetGlobal(const nString& Name, const T& Value) { Globals.At(Name.Get()) = Value; }
	template<class T> T&		GetGlobal(const nString& Name) { return Globals[Name.Get()].GetValue<T>(); }
	template<class T> bool		GetGlobal(const nString& Name, T& OutValue) const;
	bool						GetGlobal(const nString& Name, Data::CData& OutValue) const;
};

template<class T> inline bool CCoreServer::GetGlobal(const nString& Name, T& OutValue) const
{
	Data::CData Data;
	return Globals.Get(Name.Get(), Data) ? Data.GetValue<T>(OutValue) : false;
}
//---------------------------------------------------------------------

inline bool CCoreServer::GetGlobal(const nString& Name, Data::CData& OutValue) const
{
	return Globals.Get(Name.Get(), OutValue);
}
//---------------------------------------------------------------------

}

#endif
