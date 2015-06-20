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

class CIOServer: public Core::CObject
{
	__DeclareClassNoFactory;
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
	bool			MountNPK(const CString& NPKPath, const CString& Root = NULL);

	bool			FileExists(const CString& Path) const;
	bool			IsFileReadOnly(const CString& Path) const;
	bool			SetFileReadOnly(const CString& Path, bool ReadOnly) const;
	bool			DeleteFile(const CString& Path) const;
	DWORD			GetFileSize(const CString& Path) const;
	bool			CopyFile(const CString& SrcPath, const CString& DestPath);
	bool			DirectoryExists(const CString& Path) const;
	bool			CreateDirectory(const CString& Path) const;
	bool			DeleteDirectory(const CString& Path) const;
	bool			CopyDirectory(const CString& SrcPath, const CString& DestPath, bool Recursively);
	//bool Checksum(const CString& filename, uint& crc);
	//nFileTime GetFileWriteTime(const CString& pathName);

	void*			OpenFile(PFileSystem& OutFS, const CString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) const;
	void*			OpenDirectory(const CString& Path, const CString& Filter, PFileSystem& OutFS, CString& OutName, EFSEntryType& OutType) const;

	void			SetAssign(const CString& Assign, const CString& Path);
	CString			GetAssign(const CString& Assign) const;
	CString			ManglePath(const CString& Path) const;
	bool			LoadFileToBuffer(const CString& FileName, Data::CBuffer& Buffer);

#ifdef _EDITOR
	bool			QueryMangledPath(const CString& FileName, CString& MangledFileName) const;
#endif
};

}

#endif
