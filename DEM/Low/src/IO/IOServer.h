#pragma once
#include <Data/Singleton.h>
#include <IO/IOFwd.h>
#include <Data/String.h>
#include <vector>
#include <map>

// IO server manages input/output, file systems and path assigns
// NB: file times are seconds from 1970-01-01

namespace IO
{
typedef Ptr<class IFileSystem> PFileSystem;
typedef Ptr<class IStream> PStream;

#define IOSrv IO::CIOServer::Instance()

class CIOServer
{
	__DeclareSingleton(CIOServer);

private:

	struct CFSRecord
	{
		PFileSystem	FS;
		std::string		Name;
		std::string		RootPath;

		//~CFSRecord();
	};

	std::vector<CFSRecord>                  FileSystems;
	std::map<std::string, std::string, std::less<>> Assigns;

	const char*		GetFSLocalPath(const CFSRecord& Rec, const char* pPath, size_t ColonIndex) const;

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
	void*			OpenDirectory(const char* pPath, const char* pFilter, PFileSystem& OutFS, std::string& OutName, EFSEntryType& OutType) const;

	PStream			CreateStream(const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) const;

	void			SetAssign(const char* pAssign, const char* pPath);
	std::string			GetAssign(const char* pAssign) const;
	std::string			ResolveAssigns(const char* pPath) const;
};

}
