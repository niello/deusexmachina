#pragma once
#include <Data/Singleton.h>
#include <IO/IOFwd.h>
#include <Data/HashTable.h>
#include <Data/String.h>
#include <vector>

// IO server manages input/output, file systems and path assigns
// NB: file times are seconds from 1970-01-01

namespace IO
{
typedef Ptr<class IFileSystem> PFileSystem;
typedef Ptr<class CStream> PStream;

#define IOSrv IO::CIOServer::Instance()

class CIOServer
{
	__DeclareSingleton(CIOServer);

private:

	struct CFSRecord
	{
		PFileSystem	FS;
		CString		Name;
		CString		RootPath;
	};

	std::vector<CFSRecord>			FileSystems;
	CHashTable<CString, CString>	Assigns;

public:

	CIOServer();
	~CIOServer();

	bool			MountFileSystem(IO::IFileSystem* pFS, const char* pRoot, IO::IFileSystem* pPlaceBefore = nullptr);
	bool			UnmountFileSystem(IO::IFileSystem* pFS);
	UPTR			UnmountFileSystems(const char* pName);

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
};

}
