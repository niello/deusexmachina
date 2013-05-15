#pragma once
#ifndef __DEM_L1_IO_SERVER_H__
#define __DEM_L1_IO_SERVER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <IO/IO.h>
#include <util/HashMap.h>

#undef DeleteFile

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

class CIOServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CIOServer);

private:

	PFileSystem							DefaultFS;
	nArray<PFileSystem>					FS;
	CHashMap<nString>					Assigns; //!!!need better hashmap with Clear, Find etc!

public:

#ifdef _EDITOR
	CDataPathCallback					DataPathCB;
	CReleaseMemoryCallback				ReleaseMemoryCB;
#endif

	CIOServer();
	~CIOServer();

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
	bool			LoadFileToBuffer(const nString& FileName, Data::CBuffer& Buffer);

#ifdef _EDITOR
	bool			QueryMangledPath(const nString& FileName, nString& MangledFileName);
#endif
};

}

#endif
