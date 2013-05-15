#pragma once
#ifndef __DEM_L1_DATA_SERVER_H__
#define __DEM_L1_DATA_SERVER_H__

#include <Data/DataScheme.h>
#include <Core/Singleton.h>
#include <util/HashMap.h>

// Data server manages descs and data serialization schemes

//???HRD/PRM loading here, IO server don't know about file format specifics?
//???use loader objects or functions instead of server method?

//???need at all? move to IO?

namespace Data
{
typedef Ptr<class CParams> PParams;
typedef Ptr<class CXMLDocument> PXMLDocument;

#define DataSrv Data::CDataServer::Instance()

class CDataServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CDataServer);

private:

	CHashMap<PParams>					HRDCache; //!!!need better hashmap with Clear, Find etc!
	//!!!Desc cache!
	nDictionary<CStrID, PDataScheme>	DataSchemes;

public:

	CDataServer();
	~CDataServer() { __DestructSingleton; }

	PParams			LoadHRD(const nString& FileName, bool Cache = true);
	PParams			ReloadHRD(const nString& FileName, bool Cache = true);	// Force reloading from file
	void			SaveHRD(const nString& FileName, PParams Content);
	//???void			UnloadHRD(CParams* Data);
	void			UnloadHRD(const nString& FileName);
	//void			ClearHRDCache() { HRDCache. }

	PParams			LoadPRM(const nString& FileName, bool Cache = true);
	PParams			ReloadPRM(const nString& FileName, bool Cache = true);	// Force reloading from file
	void			SavePRM(const nString& FileName, PParams Content);
	
	PXMLDocument	LoadXML(const nString& FileName); //, bool Cache = true);

	bool			LoadDesc(PParams& Out, const nString& FileName, bool Cache = true);

	bool			LoadDataSchemes(const nString& FileName);
	CDataScheme*	GetDataScheme(CStrID ID);
};

inline CDataScheme* CDataServer::GetDataScheme(CStrID ID)
{
	int Idx = DataSchemes.FindIndex(ID);
	return Idx != INVALID_INDEX ? DataSchemes.ValueAtIndex(Idx) : NULL;
}
//---------------------------------------------------------------------

}

#endif
