#include "IOServer.h"
#include <IO/Streams/FileStream.h>
#include <IO/FSBrowser.h>
#include <IO/PathUtils.h>
#include <Data/StringUtils.h>

namespace IO
{
__ImplementSingleton(IO::CIOServer);

CIOServer::CIOServer()
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CIOServer::~CIOServer()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

// pRoot takes the form "Name:RootPath"
bool CIOServer::MountFileSystem(IO::IFileSystem* pFS, const char* pRoot, IO::IFileSystem* pPlaceBefore)
{
	ZoneScoped;
	ZoneText(pRoot, std::strlen(pRoot));

	if (!pFS || !pFS->Init()) FAIL;

	const char* pRootPath = strchr(pRoot, ':');

	CFSRecord Rec;
	Rec.FS = pFS;
	if (pRootPath)
	{
		Rec.Name.assign(pRoot, pRootPath - pRoot);
		Rec.RootPath.assign(pRootPath + 1);
		PathUtils::EnsurePathHasEndingDirSeparator(Rec.RootPath);
	}
	else Rec.Name.assign(pRoot);

	auto it = std::find_if(FileSystems.begin(), FileSystems.end(), [pPlaceBefore](const CFSRecord& Rec) { return Rec.FS == pPlaceBefore; });
	FileSystems.insert(it, std::move(Rec));

	OK;
}
//---------------------------------------------------------------------

bool CIOServer::UnmountFileSystem(IO::IFileSystem* pFS)
{
	NOT_IMPLEMENTED;
	FAIL; //return FileSystems.RemoveByValue(pFS);
}
//---------------------------------------------------------------------

// Unmounts all systems associated with the name provided. Returns unmounted system count.
UPTR CIOServer::UnmountFileSystems(const char* pName)
{
	NOT_IMPLEMENTED;
	return 0;
}
//---------------------------------------------------------------------

const char* CIOServer::GetFSLocalPath(const CFSRecord& Rec, const char* pPath, size_t ColonIndex) const
{
	// Invalid arguments
	if (!pPath) return nullptr;

	const char* pLocalPath;
	if (ColonIndex != std::string::npos)
	{
		// Check file system ID
		if (ColonIndex > 0 && !Rec.Name.empty() && strncmp(pPath, Rec.Name.c_str(), ColonIndex)) return nullptr;

		pLocalPath = pPath + (ColonIndex + 1);
	}
	else
	{
		pLocalPath = pPath;
	}

	const UPTR RootPathLen = Rec.RootPath.size();
	if (RootPathLen)
	{
		if (Rec.FS->IsCaseSensitive())
		{
			if (strncmp(Rec.RootPath.c_str(), pLocalPath, RootPathLen)) return nullptr;
		}
		else
		{
			if (_strnicmp(Rec.RootPath.c_str(), pLocalPath, RootPathLen)) return nullptr;
		}
		pLocalPath += RootPathLen;
	}

	if (pLocalPath[0] == '/') ++pLocalPath;

	// If local path starts with ".." (parent directory) it violates
	// the file system boundary and can't be used with this file system.
	return strncmp(pLocalPath, "..", 2) ? pLocalPath : nullptr;
}
//---------------------------------------------------------------------

bool CIOServer::FileExists(const char* pPath) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (pLocalPath && Rec.FS->FileExists(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::IsFileReadOnly(const char* pPath) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (pLocalPath && Rec.FS->FileExists(pLocalPath))
			return Rec.FS->IsFileReadOnly(pLocalPath);
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::SetFileReadOnly(const char* pPath, bool ReadOnly) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (pLocalPath && Rec.FS->FileExists(pLocalPath))
			return Rec.FS->SetFileReadOnly(pLocalPath, ReadOnly);
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteFile(const char* pPath) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (pLocalPath && Rec.FS->DeleteFile(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyFile(const char* pSrcPath, const char* pDestPath)
{
	const std::string SrcPath = ResolveAssigns(pSrcPath);
	const std::string DestPath = ResolveAssigns(pDestPath);

	// Try to copy inside a single FS

	const auto SrcColonIdx = SrcPath.find(':');
	const auto DestColonIdx = DestPath.find(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalSrcPath = GetFSLocalPath(Rec, SrcPath.c_str(), SrcColonIdx);
		const char* pLocalDestPath = GetFSLocalPath(Rec, DestPath.c_str(), DestColonIdx);
		if (Rec.FS->CopyFile(pLocalSrcPath, pLocalDestPath)) OK;
	}

	// Cross-FS copying

	if (FileExists(DestPath.c_str()) && !SetFileReadOnly(DestPath.c_str(), false)) FAIL;

	IO::PStream Src = CreateStream(SrcPath.c_str(), SAM_READ, SAP_SEQUENTIAL);
	IO::PStream Dest = CreateStream(DestPath.c_str(), SAM_WRITE, SAP_SEQUENTIAL);
	if (!Src || !Src->IsOpened()) FAIL;
	if (!Dest || !Dest->IsOpened()) FAIL;

	bool Result = true;
	U64 Size = Src->GetSize();
	const U64 MaxBytesPerOp = 16 * 1024 * 1024; // 16 MB
	void* pBuffer = std::malloc((UPTR)std::min(Size, MaxBytesPerOp));
	while (Size > 0)
	{
		UPTR CurrOpBytes = (UPTR)std::min(Size, MaxBytesPerOp);
		if (Src->Read(pBuffer, CurrOpBytes) != CurrOpBytes)
		{
			Result = false;
			FAIL;
		}
		if (Dest->Write(pBuffer, CurrOpBytes) != CurrOpBytes)
		{
			Result = false;
			FAIL;
		}
		Size -= CurrOpBytes;
	}
	Src->Close();
	Dest->Close();
	std::free(pBuffer);

	return Result;
}
//---------------------------------------------------------------------

U64 CIOServer::GetFileSize(const char* pPath) const
{
	PFileSystem FS;
	void* hFile = OpenFile(FS, pPath, SAM_READ);
	if (hFile)
	{
		U64 Size = FS->GetFileSize(hFile);
		FS->CloseFile(hFile);
		return Size;
	}
	return 0;
}
//---------------------------------------------------------------------

U64 CIOServer::GetFileWriteTime(const char* pPath) const
{
	PFileSystem FS;
	void* hFile = OpenFile(FS, pPath, SAM_READ);
	if (hFile)
	{
		U64 Time = FS->GetFileWriteTime(hFile);
		FS->CloseFile(hFile);
		return Time;
	}
	return 0;
}
//---------------------------------------------------------------------

bool CIOServer::DirectoryExists(const char* pPath) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (pLocalPath && Rec.FS->DirectoryExists(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CreateDirectory(const char* pPath) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (pLocalPath && Rec.FS->CreateDirectory(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteDirectory(const char* pPath) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (pLocalPath && Rec.FS->DeleteDirectory(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyDirectory(const char* pSrcPath, const char* pDestPath, bool Recursively)
{
	const std::string SrcBasePath = ResolveAssigns(pSrcPath);
	const std::string DestBasePath = ResolveAssigns(pDestPath);

	CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(SrcBasePath.c_str())) FAIL;

	if (!CreateDirectory(DestBasePath.c_str())) FAIL;

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			const std::string EntryName = "/" + Browser.GetCurrEntryName();
			if (!CopyFile((SrcBasePath + EntryName).c_str(), (DestBasePath + EntryName).c_str())) FAIL;
		}
		else if (Recursively && Browser.IsCurrEntryDir())
		{
			const std::string EntryName = "/" + Browser.GetCurrEntryName();
			if (!CopyDirectory((SrcBasePath + EntryName).c_str(), (DestBasePath + EntryName).c_str(), Recursively)) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

void* CIOServer::OpenFile(PFileSystem& OutFS, const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');

	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (!pLocalPath) continue;

		void* hFile = Rec.FS->OpenFile(pLocalPath, Mode, Pattern);
		if (hFile)
		{
			OutFS = Rec.FS;
			return hFile;
		}
	}

	return nullptr;
}
//---------------------------------------------------------------------

void* CIOServer::OpenDirectory(const char* pPath, const char* pFilter, PFileSystem& OutFS, std::string& OutName, EFSEntryType& OutType) const
{
	const std::string Path = ResolveAssigns(pPath);
	const auto ColonIdx = Path.find(':');

	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (!pLocalPath) continue;

		void* hDir = Rec.FS->OpenDirectory(pLocalPath, pFilter, OutName, OutType);
		if (hDir)
		{
			OutFS = Rec.FS;
			return hDir;
		}
	}

	return nullptr;
}
//---------------------------------------------------------------------

PStream CIOServer::CreateStream(const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern) const
{
	ZoneScoped;
	ZoneText(pPath, std::strlen(pPath));

	const std::string Path = ResolveAssigns(pPath);
	if (Path.empty()) return nullptr;

	const auto ColonIdx = Path.find(':');

	IFileSystem* pFS = nullptr;
	const char* pLocalPath = nullptr;

	//!!!duplicate search for NPK, finds FS record twice, here and in IStream::Open()!
	for (auto& Rec : FileSystems)
	{
		pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
		if (pLocalPath && Rec.FS->FileExists(pLocalPath))
		{
			pFS = Rec.FS;
			break;
		}
	}

	//!!!for writing new files! redesign?
	if (!pFS)
	{
		for (auto& Rec : FileSystems)
		{
			pLocalPath = GetFSLocalPath(Rec, Path.c_str(), ColonIdx);
			if (pLocalPath && !Rec.FS->IsReadOnly())
			{
				pFS = Rec.FS;
				break;
			}
		}
	}

	return pFS ? n_new(CFileStream(pLocalPath, pFS, Mode, Pattern)) : nullptr;
}
//---------------------------------------------------------------------

void CIOServer::SetAssign(const char* pAssign, const char* pPath)
{
	std::string RealAssign(pAssign);
	std::transform(RealAssign.begin(), RealAssign.end(), RealAssign.begin(), [](unsigned char c) { return std::tolower(c); });
	std::string& PathString = Assigns[RealAssign];
	PathString = pPath;
	std::replace(PathString.begin(), PathString.end(), '\\', '/');
	PathUtils::EnsurePathHasEndingDirSeparator(PathString);
}
//---------------------------------------------------------------------

std::string CIOServer::GetAssign(const char* pAssign) const
{
	std::string RealAssign(pAssign);
	std::transform(RealAssign.begin(), RealAssign.end(), RealAssign.begin(), [](unsigned char c) { return std::tolower(c); });
	auto It = Assigns.find(RealAssign); // FIXME: case-insensitive search in a map?
	return (It != Assigns.cend()) ? It->second : std::string{};
}
//---------------------------------------------------------------------

std::string CIOServer::ResolveAssigns(const char* pPath) const
{
	std::string PathString(pPath);
	std::replace(PathString.begin(), PathString.end(), '\\', '/');

	auto ColonIdx = PathString.find(':');

	// Ignore one character "assigns" because they are really DOS drive letters
	while (ColonIdx != std::string::npos && ColonIdx > 1)
	{
		std::string Assign = PathString.substr(0, ColonIdx);
		std::transform(Assign.begin(), Assign.end(), Assign.begin(), [](unsigned char c) { return std::tolower(c); });
		auto It = Assigns.find(Assign); // FIXME: case-insensitive search in a map?
		if (It == Assigns.cend()) break;
		PathString = It->second + PathString.substr(ColonIdx + 1, PathString.size() - (ColonIdx + 1));
		ColonIdx = PathString.find(':');
	}

	PathString = PathUtils::CollapseDots(PathString.c_str(), PathString.size());
	PathString = StringUtils::TrimRight(PathString, " \r\n\t\\/");
	return PathString;
}
//---------------------------------------------------------------------

}
