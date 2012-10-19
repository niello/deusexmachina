#pragma once
#ifndef __DEM_L1_DATA_SERVER_H__
#define __DEM_L1_DATA_SERVER_H__

#include <Core/RefCounted.h>
#include <Data/FileSystem.h>
#include <Data/StringID.h>
#include <Data/DataScheme.h>
#include <util/HashMap.h>
#include <util/ndictionary.h>
#include <StdDEM.h>

// Data server manages IO and data caching

namespace Data
{
class CBuffer;
typedef Ptr<class CParams> PParams;
typedef Ptr<class CXMLDocument> PXMLDocument;

#define DataSrv Data::CDataServer::Instance()

class CDataServer: public Core::CRefCounted
{
	DeclareRTTI;
	__DeclareSingleton(CDataServer);

private:

	PFileSystem							DefaultFS;
	nArray<PFileSystem>					FS;
	Ptr<class CHRDParser>				pHRDParser;
	CHashMap<PParams>					HRDCache; //!!!need better hashmap with Clear, Find etc!
	CHashMap<nString>					Assigns; //!!!need better hashmap with Clear, Find etc!
	nDictionary<CStrID, PDataScheme>	DataSchemes;

public:

	CDataServer();
	~CDataServer();

	bool			MountNPK(const nString& NPKPath, const nString& Root = NULL);

	bool			FileExists(const nString& Path) const;
	bool			IsFileReadOnly(const nString& Path) const;
	bool			SetFileReadOnly(const nString& Path, bool ReadOnly) const;
	bool			DeleteFile(const nString& Path) const;
	DWORD			GetFileSize(const nString& Path) const;
	bool			DirectoryExists(const nString& Path) const;
	bool			CreateDirectory(const nString& Path) const;
	bool			DeleteDirectory(const nString& Path) const;
	bool			CopyFile(const nString& SrcPath, const nString& DestPath);
	//bool Checksum(const nString& filename, uint& crc);
	//nFileTime GetFileWriteTime(const nString& pathName);

	void*			OpenFile(PFileSystem& OutFS, const nString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) const;
	void*			OpenDirectory(const nString& Path, const nString& Filter, PFileSystem& OutFS, nString& OutName, EFSEntryType& OutType) const;

	//???LoadXML? then rename these functions not to bind name to data format.
	void			SetAssign(const nString& Assign, const nString& Path);
	nString			GetAssign(const nString& Assign);
	nString			ManglePath(const nString& Path);
	//DWORD			LoadFileToBuffer(const nString& FileName, char*& Buffer);
	bool			LoadFileToBuffer(const nString& FileName, CBuffer& Buffer);
	
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
