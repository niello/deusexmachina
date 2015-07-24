#pragma once
#ifndef __DEM_L1_IO_SERVER_H__
#define __DEM_L1_IO_SERVER_H__

#include <Core/Object.h>
#include <Data/Singleton.h>
#include <IO/IOFwd.h>
#include <Data/HashTable.h>

// IO server manages input/output, file systems, generic file caching, path assigns

//!!!implement file caching if needed!

namespace Data
{
	class CBuffer;
}

namespace IO
{
typedef Ptr<class IFileSystem> PFileSystem;

#ifdef _EDITOR
typedef bool (__stdcall *CDataPathCallback)(LPCSTR DataPath, LPSTR* MangledPath);
typedef void (__stdcall *CReleaseMemoryCallback)(void* p);
#endif

#define IOSrv IO::CIOServer::Instance()

class CIOServer
{
	__DeclareSingleton(CIOServer);

private:

	PFileSystem							DefaultFS;
	CArray<PFileSystem>					FS;
	CHashTable<CString, CString>		Assigns;

public:

#ifdef _EDITOR
	CDataPathCallback					DataPathCB;
	CReleaseMemoryCallback				ReleaseMemoryCB;
#endif

	CIOServer();
	~CIOServer();

	//???create FSs outside and mount all with priority?
	bool			MountNPK(const char* pNPKPath, const CString& Root = CString::Empty);

	bool			FileExists(const char* pPath) const;
	bool			IsFileReadOnly(const char* pPath) const;
	bool			SetFileReadOnly(const char* pPath, bool ReadOnly) const;
	bool			DeleteFile(const char* pPath) const;
	DWORD			GetFileSize(const char* pPath) const; //???QWORD?
	bool			CopyFile(const char* pSrcPath, const char* pDestPath);
	bool			DirectoryExists(const char* pPath) const;
	bool			CreateDirectory(const char* pPath) const;
	bool			DeleteDirectory(const char* pPath) const;
	bool			CopyDirectory(const char* pSrcPath, const char* pDestPath, bool Recursively);
	//bool Checksum(const CString& filename, uint& crc);
	//nFileTime GetFileWriteTime(const CString& pathName);

	void*			OpenFile(PFileSystem& OutFS, const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) const;
	void*			OpenDirectory(const char* pPath, const char* pFilter, PFileSystem& OutFS, CString& OutName, EFSEntryType& OutType) const;

	void			SetAssign(const char* pAssign, const CString& Path);
	CString			GetAssign(const char* pAssign) const;
	CString			ManglePath(const char* pPath) const;
	bool			LoadFileToBuffer(const char* pFileName, Data::CBuffer& Buffer);

#ifdef _EDITOR
	bool			QueryMangledPath(const CString& FileName, CString& MangledFileName) const;
#endif
};

}

#endif
