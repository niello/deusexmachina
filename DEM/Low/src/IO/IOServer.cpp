#include "IOServer.h"

#include <Data/DataBuffer.h>
#include <IO/Streams/FileStream.h>
#include <IO/FSBrowser.h>
#include <IO/PathUtils.h>

namespace IO
{
__ImplementSingleton(IO::CIOServer);

//CIOServer::CFSRecord::~CFSRecord() = default;

CIOServer::CIOServer(): Assigns(32)
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
	if (!pFS || !pFS->Init()) FAIL;

	const char* pRootPath = strchr(pRoot, ':');

	CFSRecord Rec;
	Rec.FS = pFS;
	if (pRootPath)
	{
		Rec.Name.Set(pRoot, pRootPath - pRoot);
		Rec.RootPath.Set(pRootPath + 1);
		PathUtils::EnsurePathHasEndingDirSeparator(Rec.RootPath);
	}
	else Rec.Name.Set(pRoot);

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

const char* CIOServer::GetFSLocalPath(const CFSRecord& Rec, const char* pPath, IPTR ColonIndex) const
{
	// Invalid arguments
	if (!pPath || ColonIndex < -1) return nullptr;

	if (ColonIndex > 0 && Rec.Name.IsValid() && strncmp(pPath, Rec.Name.CStr(), ColonIndex)) return nullptr;

	const char* pLocalPath = pPath + (ColonIndex + 1);

	const UPTR RootPathLen = Rec.RootPath.GetLength();
	if (RootPathLen)
	{
		if (Rec.FS->IsCaseSensitive())
		{
			if (strncmp(Rec.RootPath.CStr(), pLocalPath, RootPathLen)) return nullptr;
		}
		else
		{
			if (_strnicmp(Rec.RootPath.CStr(), pLocalPath, RootPathLen)) return nullptr;
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
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
		if (pLocalPath && Rec.FS->FileExists(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::IsFileReadOnly(const char* pPath) const
{
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
		if (pLocalPath && Rec.FS->FileExists(pLocalPath))
			return Rec.FS->IsFileReadOnly(pLocalPath);
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::SetFileReadOnly(const char* pPath, bool ReadOnly) const
{
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
		if (pLocalPath && Rec.FS->FileExists(pLocalPath))
			return Rec.FS->SetFileReadOnly(pLocalPath, ReadOnly);
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteFile(const char* pPath) const
{
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
		if (pLocalPath && Rec.FS->DeleteFile(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyFile(const char* pSrcPath, const char* pDestPath)
{
	const CString SrcPath = ResolveAssigns(pSrcPath);
	const CString DestPath = ResolveAssigns(pDestPath);

	// Try to copy inside a single FS

	const IPTR SrcColonIdx = SrcPath.FindIndex(':');
	const IPTR DestColonIdx = DestPath.FindIndex(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalSrcPath = GetFSLocalPath(Rec, SrcPath.CStr(), SrcColonIdx);
		const char* pLocalDestPath = GetFSLocalPath(Rec, DestPath.CStr(), DestColonIdx);
		if (Rec.FS->CopyFile(pLocalSrcPath, pLocalDestPath)) OK;
	}

	// Cross-FS copying

	if (FileExists(DestPath) && !SetFileReadOnly(DestPath, false)) FAIL;

	IO::PStream Src = CreateStream(SrcPath, SAM_READ, SAP_SEQUENTIAL);
	IO::PStream Dest = CreateStream(DestPath, SAM_WRITE, SAP_SEQUENTIAL);
	if (!Src || !Src->IsOpened()) FAIL;
	if (!Dest || !Dest->IsOpened()) FAIL;

	bool Result = true;
	U64 Size = Src->GetSize();
	const U64 MaxBytesPerOp = 16 * 1024 * 1024; // 16 MB
	void* pBuffer = n_malloc((UPTR)std::min(Size, MaxBytesPerOp));
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
	n_free(pBuffer);

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
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
		if (pLocalPath && Rec.FS->DirectoryExists(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CreateDirectory(const char* pPath) const
{
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
		if (pLocalPath && Rec.FS->CreateDirectory(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteDirectory(const char* pPath) const
{
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');
	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
		if (pLocalPath && Rec.FS->DeleteDirectory(pLocalPath)) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyDirectory(const char* pSrcPath, const char* pDestPath, bool Recursively)
{
	const CString SrcBasePath = ResolveAssigns(pSrcPath);
	const CString DestBasePath = ResolveAssigns(pDestPath);

	CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(SrcBasePath)) FAIL;

	if (!CreateDirectory(DestBasePath)) FAIL;

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			const CString EntryName = "/" + Browser.GetCurrEntryName();
			if (!CopyFile(SrcBasePath + EntryName, DestBasePath + EntryName)) FAIL;
		}
		else if (Recursively && Browser.IsCurrEntryDir())
		{
			const CString EntryName = "/" + Browser.GetCurrEntryName();
			if (!CopyDirectory(SrcBasePath + EntryName, DestBasePath + EntryName, Recursively)) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

void* CIOServer::OpenFile(PFileSystem& OutFS, const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern) const
{
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');

	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
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

void* CIOServer::OpenDirectory(const char* pPath, const char* pFilter, PFileSystem& OutFS, CString& OutName, EFSEntryType& OutType) const
{
	const CString Path = ResolveAssigns(pPath);
	const IPTR ColonIdx = Path.FindIndex(':');

	for (auto& Rec : FileSystems)
	{
		const char* pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
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
	const CString Path = ResolveAssigns(pPath);
	if (Path.IsEmpty()) return nullptr;

	const IPTR ColonIdx = Path.FindIndex(':');

	IFileSystem* pFS = nullptr;
	const char* pLocalPath = nullptr;

	//!!!duplicate search for NPK, finds FS record twice, here and in IStream::Open()!
	for (auto& Rec : FileSystems)
	{
		pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
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
			pLocalPath = GetFSLocalPath(Rec, Path.CStr(), ColonIdx);
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
	CString RealAssign(pAssign);
	RealAssign.ToLower();
	CString& PathString = Assigns.At(RealAssign);
	PathString = pPath;
	PathString.Replace('\\', '/');
	PathUtils::EnsurePathHasEndingDirSeparator(PathString);
}
//---------------------------------------------------------------------

CString CIOServer::GetAssign(const char* pAssign) const
{
	CString RealAssign(pAssign);
	RealAssign.ToLower();
	CString Str;
	return Assigns.Get(RealAssign, Str) ? Str : CString::Empty;
}
//---------------------------------------------------------------------

CString CIOServer::ResolveAssigns(const char* pPath) const
{
	CString PathString(pPath);
	PathString.Replace('\\', '/');

	IPTR ColonIdx;

	// Ignore one character "assigns" because they are really DOS drive letters
	while ((ColonIdx = PathString.FindIndex(':')) > 1)
	{
		CString Assign = PathString.SubString(0, ColonIdx);
		Assign.ToLower();
		CString AssignValue;
		if (!Assigns.Get(Assign, AssignValue)) break;
		PathString = AssignValue + PathString.SubString(ColonIdx + 1, PathString.GetLength() - (ColonIdx + 1));
	}

	PathString = PathUtils::CollapseDots(PathString.CStr(), PathString.GetLength());
	PathString.Trim(" \r\n\t\\/", false);
	return PathString;
}
//---------------------------------------------------------------------

}