#pragma once
#ifndef __DEM_L1_DATA_SERVER_H__
#define __DEM_L1_DATA_SERVER_H__

#include <Core/Object.h>
#include <Data/DataScheme.h>
#include <Data/Dictionary.h>
#include <Data/Singleton.h>
#include <Data/HashTable.h>

// Data server manages descs and data serialization schemes
//!!!subject to removal!

//???HRD/PRM - use loader objects or functions instead of server method?

namespace Data
{
typedef Ptr<class CParams> PParams;

#define DataSrv Data::CDataServer::Instance()

class CDataServer
{
	__DeclareSingleton(CDataServer);

private:

	CHashTable<CString, PParams>	HRDCache; //!!!need better hashmap with Clear, Find etc!
	//!!!Desc cache!
	CDict<CStrID, PDataScheme>		DataSchemes;

public:

	CDataServer();
	~CDataServer() { __DestructSingleton; }

	//???to resource managers? the same cache can br implemented. or special class C...Cache?
	PParams			LoadHRD(const char* pFileName, bool Cache = true);
	PParams			ReloadHRD(const char* pFileName, bool Cache = true);	// Force reloading from file
	void			SaveHRD(const char* pFileName, const CParams* pContent);
	//???void			UnloadHRD(CParams* Data);
	void			UnloadHRD(const char* pFileName);
	//void			ClearHRDCache() { HRDCache. }

	PParams			LoadPRM(const char* pFileName, bool Cache = true);
	PParams			ReloadPRM(const char* pFileName, bool Cache = true);	// Force reloading from file
	bool			SavePRM(const char* pFileName, const CParams* pContent);

	bool			LoadDesc(PParams& Out, const char* pContext, const char* pName, bool Cache = true);

	bool			LoadDataSchemes(const char* pFileName);
	CDataScheme*	GetDataScheme(CStrID ID);
};

inline CDataScheme* CDataServer::GetDataScheme(CStrID ID)
{
	IPTR Idx = DataSchemes.FindIndex(ID);
	return Idx != INVALID_INDEX ? DataSchemes.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

}

#endif
