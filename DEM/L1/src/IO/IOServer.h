#pragma once
#ifndef __DEM_L1_IO_SERVER_H__
#define __DEM_L1_IO_SERVER_H__

#include <Core/Object.h>
#include <Data/Singleton.h>
#include <IO/IOFwd.h>
#include <Data/HashTable.h>

// IO server manages input/output, file systems, generic file caching, path assigns
// NB: file times are seconds from 1970-01-01

//!!!implement file caching if needed!

namespace Data
{
	class CBuffer;
}

namespace IO
{
typedef Ptr<class IFileSystem> PFileSystem;
typedef Ptr<class CStream> PStream;

#define IOSrv IO::CIOServer::Instance()

class CIOServer
{
	__DeclareSingleton(CIOServer);

private:

	PFileSystem							DefaultFS; //???need? use FS[0] and priority-sorted array?
	CArray<PFileSystem>					FS;
	CHashTable<CString, CString>		Assigns;

public:

	CIOServer();
	~CIOServer() { __DestructSingleton; }

	//???create FSs outside and mount all with priority?
	bool			MountNPK(const char* pNPKPath, const char* pRoot = NULL);

	bool			FileExists(const char* pPath) const;
	bool			IsFileReadOnly(const char* pPath) const;
	bool			SetFileReadOnly(const char* pPath, bool ReadOnly) const;
	bool			DeleteFile(const char* pPath) const;
	U64				GetFileSize(const char* pPath) const;
	U64				GetFileWriteTime(const char* pPath) const;
	bool			CopyFile(const char* pSrcPath, const char* pDestPath);
	bool			DirectoryExists(const char* pPath) const;
	bool			CreateDirectory(const char* pPath) const;
	bool			DeleteDirectory(const char* pPath) const;
	bool			CopyDirectory(const char* pSrcPath, const char* pDestPath, bool Recursively);

	void*			OpenFile(PFileSystem& OutFS, const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) const;
	void*			OpenDirectory(const char* pPath, const char* pFilter, PFileSystem& OutFS, CString& OutName, EFSEntryType& OutType) const;

	PStream			CreateStream(const char* pURI) const;

	void			SetAssign(const char* pAssign, const char* pPath);
	CString			GetAssign(const char* pAssign) const;
	CString			ResolveAssigns(const char* pPath) const;
	bool			LoadFileToBuffer(const char* pFileName, Data::CBuffer& Buffer);
};

}

#endif
