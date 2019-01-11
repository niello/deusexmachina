#include "IOServer.h"

#include <Data/Buffer.h>
#include <IO/Streams/FileStream.h>
#include <IO/FSBrowser.h>
#include <IO/PathUtils.h>

namespace IO
{
__ImplementSingleton(IO::CIOServer);

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
	}
	else Rec.Name.Set(pRoot);

	auto it = std::find_if(FileSystems.begin(), FileSystems.end(), [pPlaceBefore](const CFSRecord& Rec) { return Rec.FS == pPlaceBefore; });
	FileSystems.insert(it, std::move(Rec));

	OK;
}
//---------------------------------------------------------------------

bool CIOServer::UnmountFileSystem(IO::IFileSystem* pFS)
{
	FAIL; //return FileSystems.RemoveByValue(pFS);
}
//---------------------------------------------------------------------

// Unmounts all systems associated with the name provided. Returns unmounted system count.
UPTR CIOServer::UnmountFileSystems(const char* pName)
{
	return 0;
}
//---------------------------------------------------------------------

bool CIOServer::FileExists(const char* pPath) const
{
	CString AbsPath = ResolveAssigns(pPath);
	for (auto& Rec : FileSystems)
		if (Rec.FS->FileExists(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::IsFileReadOnly(const char* pPath) const
{
	CString AbsPath = ResolveAssigns(pPath);
	for (auto& Rec : FileSystems)
		if (Rec.FS->FileExists(AbsPath))
			return Rec.FS->IsFileReadOnly(AbsPath);
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::SetFileReadOnly(const char* pPath, bool ReadOnly) const
{
	CString AbsPath = ResolveAssigns(pPath);
	for (auto& Rec : FileSystems)
		if (Rec.FS->FileExists(AbsPath))
			return Rec.FS->SetFileReadOnly(AbsPath, ReadOnly);
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteFile(const char* pPath) const
{
	CString AbsPath = ResolveAssigns(pPath);
	for (auto& Rec : FileSystems)
		if (Rec.FS->DeleteFile(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyFile(const char* pSrcPath, const char* pDestPath)
{
	CString AbsSrcPath = ResolveAssigns(pSrcPath);
	CString AbsDestPath = ResolveAssigns(pDestPath);

	// Try to copy inside a single FS

	for (auto& Rec : FileSystems)
		if (Rec.FS->CopyFile(AbsSrcPath, AbsDestPath)) OK;

	// Cross-FS copying

	if (!SetFileReadOnly(AbsDestPath, false)) FAIL;

	IO::PStream Src = CreateStream(AbsSrcPath);
	IO::PStream Dest = CreateStream(AbsDestPath);
	if (!Src->Open(SAM_READ, SAP_SEQUENTIAL)) FAIL;
	if (!Dest->Open(SAM_WRITE, SAP_SEQUENTIAL)) FAIL;

	bool Result = true;
	U64 Size = Src->GetSize();
	const U64 MaxBytesPerOp = 16 * 1024 * 1024; // 16 MB
	void* pBuffer = n_malloc((UPTR)n_min(Size, MaxBytesPerOp));
	while (Size > 0)
	{
		UPTR CurrOpBytes = (UPTR)n_min(Size, MaxBytesPerOp);
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
	CString AbsPath = ResolveAssigns(pPath);
	for (auto& Rec : FileSystems)
		if (Rec.FS->DirectoryExists(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CreateDirectory(const char* pPath) const
{
	CString AbsPath = ResolveAssigns(pPath);
	for (auto& Rec : FileSystems)
		if (Rec.FS->CreateDirectory(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteDirectory(const char* pPath) const
{
	CString AbsPath = ResolveAssigns(pPath);
	for (auto& Rec : FileSystems)
		if (Rec.FS->DeleteDirectory(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyDirectory(const char* pSrcPath, const char* pDestPath, bool Recursively)
{
	CString AbsSrcPath = ResolveAssigns(pSrcPath);
	CString AbsDestPath = ResolveAssigns(pDestPath);

	CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(AbsSrcPath)) FAIL;

	if (!CreateDirectory(AbsDestPath)) FAIL;

	if (!Browser.IsCurrDirEmpty()) do
	{
		CString EntryName = "/" + Browser.GetCurrEntryName();
		if (Browser.IsCurrEntryFile())
		{
			if (!CopyFile(AbsSrcPath + EntryName, AbsDestPath + EntryName)) FAIL;
		}
		else if (Recursively && Browser.IsCurrEntryDir())
		{
			if (!CopyDirectory(AbsSrcPath + EntryName, AbsDestPath + EntryName, Recursively)) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

void* CIOServer::OpenFile(PFileSystem& OutFS, const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern Pattern) const
{
	CString AbsPath = ResolveAssigns(pPath);

	for (auto& Rec : FileSystems)
	{
		void* hFile = Rec.FS->OpenFile(AbsPath, Mode, Pattern);
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
	CString AbsPath = ResolveAssigns(pPath);

	for (auto& Rec : FileSystems)
	{
		void* hDir = Rec.FS->OpenDirectory(AbsPath, pFilter, OutName, OutType);
		if (hDir)
		{
			OutFS = Rec.FS;
			return hDir;
		}
	}

	return nullptr;
}
//---------------------------------------------------------------------

//!!!need different schemas, like http:, ftp:, npk:, file: etc!
//default - file at least for now
//!!!for CreateStream() need to determine file system without opening a file!
PStream CIOServer::CreateStream(const char* pURI) const
{
	CString AbsURI = ResolveAssigns(pURI);
	if (AbsURI.IsEmpty()) return nullptr;

	IFileSystem* pFS = nullptr;

	//!!!duplicate search for NPK, finds FS record twice, here and in CStream::Open()!
	for (auto& Rec : FileSystems)
	{
		IFileSystem* pCurrFS = Rec.FS.Get();
		if (pCurrFS && pCurrFS->FileExists(AbsURI.CStr()))
		{
			pFS = pCurrFS;
			break;
		}
	}

	//!!!for writing new files! redesign?
	if (!pFS)
	{
		for (auto& Rec : FileSystems)
		{
			IFileSystem* pCurrFS = Rec.FS.Get();
			if (pCurrFS && !pCurrFS->IsReadOnly())
			{
				pFS = pCurrFS;
				break;
			}
		}
	}

	return n_new(CFileStream(AbsURI.CStr(), pFS));
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
		if (!Assigns.Get(Assign, AssignValue)) return CString::Empty;
		PathString = AssignValue + PathString.SubString(ColonIdx + 1, PathString.GetLength() - (ColonIdx + 1));
	}

	PathString.Trim(" \r\n\t\\/", false);
	return PathString;
}
//---------------------------------------------------------------------

}