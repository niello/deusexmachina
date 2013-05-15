#pragma once
#ifndef __DEM_L1_DB_SERVER_H__
#define __DEM_L1_DB_SERVER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <DB/AttrID.h>

// DB server manages AttrIDs (and does nothing else for now)

namespace DB
{
#define DBSrv DB::CDBServer::Instance()

class CDBServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CDBServer);

private:

	CHashTable<CStrID, CAttributeID>	AttrIDRegistry;

public:

	CDBServer();
	virtual ~CDBServer();

	CAttrID	RegisterAttrID(const char* Name, /*fourcc,*/ char Flags, const CData& DefaultVal);
	void	UnregisterAttrID(CStrID Name) { AttrIDRegistry.Erase(Name); }
	CAttrID	FindAttrID(LPCSTR Name) { return AttrIDRegistry.Get(CStrID(Name)); }
	bool	IsValidAttrName(const nString& Name) { return AttrIDRegistry.Contains(CStrID(Name.CStr())); }
}; 

}

#endif