#include "IOServer.h"

#include <Data/Buffer.h>
#include <IO/FS/FileSystemWin32.h>
#include <IO/FS/FileSystemNPK.h>
#include <IO/FSBrowser.h>
#include <IO/PathUtils.h>

namespace IO
{
__ImplementSingleton(IO::CIOServer);

CIOServer::CIOServer(): Assigns(32)
{
	__ConstructSingleton;

#ifdef _EDITOR
	DataPathCB = NULL;
#endif

	DefaultFS = n_new(CFileSystemWin32);

	CString SysFolder;
	if (DefaultFS->GetSystemFolderPath(SF_HOME, SysFolder))	SetAssign("Home", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_BIN, SysFolder))	SetAssign("Bin", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_USER, SysFolder))	SetAssign("User", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_TEMP, SysFolder))	SetAssign("Temp", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_APP_DATA, SysFolder))	SetAssign("AppData", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_PROGRAMS, SysFolder))	SetAssign("Programs", SysFolder);
}
//---------------------------------------------------------------------

CIOServer::~CIOServer()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CIOServer::MountNPK(const char* pNPKPath, const char* pRoot)
{
	PFileSystem NewFS = n_new(CFileSystemNPK);

	CString AbsNPKPath = IOSrv->ResolveAssigns(pNPKPath);
	//!!!check if this NPK is already mounted!

	CString RealRoot;
	if (pRoot && *pRoot) RealRoot = IOSrv->ResolveAssigns(pRoot);
	else RealRoot = PathUtils::ExtractDirName(AbsNPKPath);

	if (!NewFS->Mount(AbsNPKPath, RealRoot)) FAIL;
	FS.Add(NewFS);
	OK;
}
//---------------------------------------------------------------------

bool CIOServer::FileExists(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->FileExists(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::IsFileReadOnly(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->FileExists(AbsPath))
		return DefaultFS->IsFileReadOnly(AbsPath);
	for (int i = 0; i < FS.GetCount(); ++i)
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
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(AbsPath))
			return FS[i]->SetFileReadOnly(AbsPath, ReadOnly);
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteFile(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->DeleteFile(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
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
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->CopyFile(AbsSrcPath, AbsDestPath)) OK;

	// Cross-FS copying

	if (IsFileReadOnly(AbsDestPath) && !SetFileReadOnly(AbsDestPath, false)) FAIL;

	CFileStream Src, Dest;
	if (!Src.Open(AbsSrcPath, SAM_READ, SAP_SEQUENTIAL)) FAIL;
	if (!Dest.Open(AbsDestPath, SAM_WRITE, SAP_SEQUENTIAL)) FAIL;

	DWORD Size = Src.GetSize();
	void* pBuffer = n_malloc(Size);
	int RealSize = Src.Read(pBuffer, Size);
	n_assert(RealSize == Size);
	Src.Close();

	RealSize = Dest.Write(pBuffer, Size);
	n_assert(RealSize == Size);
	Dest.Close();
	n_free(pBuffer);

	OK;
}
//---------------------------------------------------------------------

//???QWORD?
DWORD CIOServer::GetFileSize(const char* pPath) const
{
	PFileSystem FS;
	void* hFile = OpenFile(FS, pPath, SAM_READ);
	if (hFile)
	{
		DWORD Size = FS->GetFileSize(hFile);
		FS->CloseFile(hFile);
		return Size;
	}
	return 0;
}
//---------------------------------------------------------------------

bool CIOServer::DirectoryExists(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->DirectoryExists(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DirectoryExists(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CreateDirectory(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->CreateDirectory(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->CreateDirectory(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteDirectory(const char* pPath) const
{
	CString AbsPath = IOSrv->ResolveAssigns(pPath);
	if (DefaultFS->DeleteDirectory(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
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

	for (int i = 0; i < FS.GetCount(); ++i)
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

	for (int i = 0; i < FS.GetCount(); ++i)
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

void CIOServer::SetAssign(const char* pAssign, const char* pPath)
{
	CString RealAssign(pAssign);
	RealAssign.ToLower();
	CString& PathString = Assigns.At(RealAssign);
	PathString = pPath;
	PathString.Replace('\\', '/');
	if (PathString[PathString.GetLength() - 1] != '/') PathString.Add('/');
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

	int ColonIdx;

	// Ignore one character "assigns" because they are really DOS drive letters
	while ((ColonIdx = PathString.FindIndex(':')) > 1)
	{
#ifdef _EDITOR
		if (QueryMangledPath(PathString, PathString)) continue;
#endif
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
	CFileStream File;
	if (!File.Open(pFileName, SAM_READ, SAP_SEQUENTIAL)) FAIL;
	int FileSize = File.GetSize();
	Buffer.Reserve(FileSize);
	Buffer.Trim(File.Read(Buffer.GetPtr(), FileSize));
	//Sys::Log("FileIO: File \"%s\" successfully loaded from HDD\n", FileName.CStr());
	return Buffer.GetSize() == FileSize;
}
//---------------------------------------------------------------------

#ifdef _EDITOR
bool CIOServer::QueryMangledPath(const char* pFileName, CString& MangledFileName) const
{
	if (!DataPathCB) FAIL;

	LPSTR pMangledStr = NULL;
	if (!DataPathCB(pFileName, &pMangledStr))
	{
		if (pMangledStr && ReleaseMemoryCB) ReleaseMemoryCB(pMangledStr);
		FAIL;
	}

	MangledFileName.Clear();
	if (pMangledStr)
	{
		MangledFileName.Set(pMangledStr);
		if (ReleaseMemoryCB) ReleaseMemoryCB(pMangledStr);
	}
	OK;
}
#endif

} //namespace Data
