#include "IOServer.h"

#include <Data/Buffer.h>
#include <IO/FS/FileSystemWin32.h>
#include <IO/FS/FileSystemNPK.h>
#include <IO/Streams/FileStream.h>
#include <IO/FSBrowser.h>
#include <IO/PathUtils.h>

namespace IO
{
__ImplementSingleton(IO::CIOServer);

CIOServer::CIOServer(): Assigns(32)
{
	__ConstructSingleton;

	//!!!move to app!
#if DEM_PLATFORM_WIN32
	DefaultFS = n_new(CFileSystemWin32(""));
#else
#error "Default FS for the target OS not defined"
#endif

	//CString SysFolder;
	//if (DefaultFS->GetSystemFolderPath(SF_HOME, SysFolder))	SetAssign("Home", SysFolder);
	//if (DefaultFS->GetSystemFolderPath(SF_BIN, SysFolder))	SetAssign("Bin", SysFolder);
	//if (DefaultFS->GetSystemFolderPath(SF_USER, SysFolder))	SetAssign("User", SysFolder);
	//if (DefaultFS->GetSystemFolderPath(SF_TEMP, SysFolder))	SetAssign("Temp", SysFolder);
	//if (DefaultFS->GetSystemFolderPath(SF_APP_DATA, SysFolder))	SetAssign("AppData", SysFolder);
	//if (DefaultFS->GetSystemFolderPath(SF_PROGRAMS, SysFolder))	SetAssign("Programs", SysFolder);
}
//---------------------------------------------------------------------

CIOServer::~CIOServer()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CIOServer::MountNPK(const char* pNPKPath, const char* pRoot)
{
	PFileSystem NewFS = n_new(CFileSystemNPK(pNPKPath));

	CString AbsNPKPath = IOSrv->ResolveAssigns(pNPKPath);
	//!!!check if this NPK is already mounted!

	CString RealRoot;
	if (pRoot && *pRoot) RealRoot = IOSrv->ResolveAssigns(pRoot);
	else RealRoot = PathUtils::ExtractDirName(AbsNPKPath);

	//if (!NewFS->Mount(AbsNPKPath, RealRoot)) FAIL;
	//FS.Add(NewFS);
	OK;
}
//---------------------------------------------------------------------

bool CIOServer::FileExists(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->FileExists(AbsPath)) OK;
	for (UPTR i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::IsFileReadOnly(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->FileExists(AbsPath))
		return DefaultFS->IsFileReadOnly(AbsPath);
	for (UPTR i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(AbsPath))
			return FS[i]->IsFileReadOnly(AbsPath);
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::SetFileReadOnly(const char* pPath, bool ReadOnly) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->FileExists(AbsPath))
		return DefaultFS->SetFileReadOnly(AbsPath, ReadOnly);
	for (UPTR i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(AbsPath))
			return FS[i]->SetFileReadOnly(AbsPath, ReadOnly);
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteFile(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->DeleteFile(AbsPath)) OK;
	for (UPTR i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DeleteFile(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyFile(const char* pSrcPath, const char* pDestPath)
{
	CString AbsSrcPath = ResolveAssigns(pSrcPath);
	CString AbsDestPath = ResolveAssigns(pDestPath);

	// Try to copy inside a single FS

	if (DefaultFS->CopyFile(AbsSrcPath, AbsDestPath)) OK;
	for (UPTR i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->CopyFile(AbsSrcPath, AbsDestPath)) OK;

	// Cross-FS copying

	if (!SetFileReadOnly(AbsDestPath, false)) FAIL;

	IO::PStream Src = IOSrv->CreateStream(AbsSrcPath);
	IO::PStream Dest = IOSrv->CreateStream(AbsDestPath);
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
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->DirectoryExists(AbsPath)) OK;
	for (UPTR i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DirectoryExists(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CreateDirectory(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->CreateDirectory(AbsPath)) OK;
	for (UPTR i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->CreateDirectory(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteDirectory(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->DeleteDirectory(AbsPath)) OK;
	for (UPTR i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DeleteDirectory(AbsPath)) OK;
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

	void* hFile = DefaultFS->OpenFile(AbsPath, Mode, Pattern);
	if (hFile)
	{
		OutFS = DefaultFS;
		return hFile;
	}

	for (UPTR i = 0; i < FS.GetCount(); ++i)
	{
		hFile = FS[i]->OpenFile(AbsPath, Mode, Pattern);
		if (hFile)
		{
			OutFS = FS[i];
			return hFile;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------

void* CIOServer::OpenDirectory(const char* pPath, const char* pFilter, PFileSystem& OutFS, CString& OutName, EFSEntryType& OutType) const
{
	CString AbsPath = ResolveAssigns(pPath);

	void* hDir = DefaultFS->OpenDirectory(AbsPath, pFilter, OutName, OutType);
	if (hDir)
	{
		OutFS = DefaultFS;
		return hDir;
	}

	for (UPTR i = 0; i < FS.GetCount(); ++i)
	{
		hDir = FS[i]->OpenDirectory(AbsPath, pFilter, OutName, OutType);
		if (hDir)
		{
			OutFS = FS[i];
			return hDir;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------

//!!!need different schemas, like http:, ftp:, npk:, file: etc!
//default - file at least for now
//!!!for CreateStream() need to determine file system without opening a file!
PStream CIOServer::CreateStream(const char* pURI) const
{
	CString AbsURI = ResolveAssigns(pURI);
	if (AbsURI.IsEmpty()) return NULL;

	IFileSystem* pFS = NULL;

	//!!!duplicate search for NPK, finds FS record twice, here and in CStream::Open()!
	if (DefaultFS->FileExists(AbsURI.CStr()))
	{
		pFS = DefaultFS.Get();
	}
	else
	{
		for (UPTR i = 0; i < FS.GetCount(); ++i)
		{
			IFileSystem* pCurrFS = FS[i].Get();
			if (pCurrFS && pCurrFS->FileExists(AbsURI.CStr()))
			{
				pFS = pCurrFS;
				break;
			}
		}
	}

	//!!!for writing new files! redesign?
	if (!pFS)
	{
		if (!DefaultFS->IsReadOnly())
		{
			pFS = DefaultFS.Get();
		}
		else
		{
			for (UPTR i = 0; i < FS.GetCount(); ++i)
			{
				IFileSystem* pCurrFS = FS[i].Get();
				if (pCurrFS && !pCurrFS->IsReadOnly())
				{
					pFS = pCurrFS;
					break;
				}
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

bool CIOServer::LoadFileToBuffer(const char* pFileName, Data::CBuffer& Buffer)
{
	IO::PStream File = CreateStream(pFileName);
	if (!File->Open(SAM_READ, SAP_SEQUENTIAL)) FAIL;
	UPTR FileSize = (UPTR)File->GetSize();
	Buffer.Reserve(FileSize);
	Buffer.Trim(File->Read(Buffer.GetPtr(), FileSize));
	return Buffer.GetSize() == FileSize;
}
//---------------------------------------------------------------------

}