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

	CDict<CStrID, PDataScheme>		DataSchemes;

public:

	CDataServer() { __ConstructSingleton; }
	~CDataServer() { __DestructSingleton; }

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
