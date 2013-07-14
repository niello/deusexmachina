#include "IOServer.h"

#include <Data/Buffer.h>
#include <IO/FS/FileSystemWin32.h>
#include <IO/FS/FileSystemNPK.h>
#include <IO/FSBrowser.h>

namespace IO
{
__ImplementClassNoFactory(IO::CIOServer, Core::CRefCounted);
__ImplementSingleton(IO::CIOServer);

CIOServer::CIOServer(): Assigns(nString())
{
	__ConstructSingleton;

#ifdef _EDITOR
	DataPathCB = NULL;
#endif

	DefaultFS = n_new(CFileSystemWin32);

	nString SysFolder;
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

bool CIOServer::MountNPK(const nString& NPKPath, const nString& Root)
{
	PFileSystem NewFS = n_new(CFileSystemNPK);

	nString AbsNPKPath = IOSrv->ManglePath(NPKPath);
	//!!!check if this NPK is already mounted!

	nString RealRoot;
	if (Root.IsValid()) RealRoot = IOSrv->ManglePath(Root);
	else RealRoot = AbsNPKPath.ExtractDirName();

	if (!NewFS->Mount(AbsNPKPath, RealRoot)) FAIL;
	FS.Add(NewFS);
	OK;
}
//---------------------------------------------------------------------

bool CIOServer::FileExists(const nString& Path) const
{
	nString AbsPath = IOSrv->ManglePath(Path);
	if (DefaultFS->FileExists(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::IsFileReadOnly(const nString& Path) const
{
	nString AbsPath = IOSrv->ManglePath(Path);
	//if (DefaultFS->FileExists(AbsPath))
		return DefaultFS->IsFileReadOnly(AbsPath);
	for (int i = 0; i < FS.GetCount(); ++i)
		//if (FS[i]->FileExists(AbsPath))
			return FS[i]->IsFileReadOnly(AbsPath);
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::SetFileReadOnly(const nString& Path, bool ReadOnly) const
{
	nString AbsPath = IOSrv->ManglePath(Path);
	//if (DefaultFS->FileExists(AbsPath))
		return DefaultFS->SetFileReadOnly(AbsPath, ReadOnly);
	for (int i = 0; i < FS.GetCount(); ++i)
		//if (FS[i]->FileExists(AbsPath))
			return FS[i]->SetFileReadOnly(AbsPath, ReadOnly);
	FAIL;
}
//---------------------------------------------------------------------

#undef DeleteFile
bool CIOServer::DeleteFile(const nString& Path) const
{
	nString AbsPath = IOSrv->ManglePath(Path);
	if (DefaultFS->DeleteFile(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DeleteFile(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyFile(const nString& SrcPath, const nString& DestPath)
{
	nString AbsSrcPath = ManglePath(SrcPath);
	nString AbsDestPath = ManglePath(DestPath);

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

DWORD CIOServer::GetFileSize(const nString& Path) const
{
	PFileSystem FS;
	void* hFile = OpenFile(FS, Path, SAM_READ);
	if (hFile)
	{
		DWORD Size = FS->GetFileSize(hFile);
		FS->CloseFile(hFile);
		return Size;
	}
	return 0;
}
//---------------------------------------------------------------------

bool CIOServer::DirectoryExists(const nString& Path) const
{
	nString AbsPath = IOSrv->ManglePath(Path);
	if (DefaultFS->DirectoryExists(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DirectoryExists(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CreateDirectory(const nString& Path) const
{
	nString AbsPath = IOSrv->ManglePath(Path);
	if (DefaultFS->CreateDirectory(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->CreateDirectory(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteDirectory(const nString& Path) const
{
	nString AbsPath = IOSrv->ManglePath(Path);
	if (DefaultFS->DeleteDirectory(AbsPath)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DeleteDirectory(AbsPath)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CopyDirectory(const nString& SrcPath, const nString& DestPath, bool Recursively)
{
	nString AbsSrcPath = ManglePath(SrcPath);
	nString AbsDestPath = ManglePath(DestPath);

	CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(AbsSrcPath)) FAIL;

	if (!CreateDirectory(AbsDestPath)) FAIL;

	if (!Browser.IsCurrDirEmpty()) do
	{
		nString EntryName = "/" + Browser.GetCurrEntryName();
		if (Browser.IsCurrEntryFile())
		{
			if (!CopyFile(SrcPath + EntryName, DestPath + EntryName)) FAIL;
		}
		else if (Recursively && Browser.IsCurrEntryDir())
		{
			if (!CopyDirectory(SrcPath + EntryName, DestPath + EntryName, Recursively)) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

void* CIOServer::OpenFile(PFileSystem& OutFS, const nString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern) const
{
	nString AbsPath = ManglePath(Path);

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

void* CIOServer::OpenDirectory(const nString& Path, const nString& Filter,
								 PFileSystem& OutFS, nString& OutName, EFSEntryType& OutType) const
{
	nString AbsPath = ManglePath(Path);

	void* hDir = DefaultFS->OpenDirectory(AbsPath, Filter, OutName, OutType);
	if (hDir)
	{
		OutFS = DefaultFS;
		return hDir;
	}

	for (int i = 0; i < FS.GetCount(); ++i)
	{
		hDir = FS[i]->OpenDirectory(AbsPath, Filter, OutName, OutType);
		if (hDir)
		{
			OutFS = FS[i];
			return hDir;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------

void CIOServer::SetAssign(const nString& Assign, const nString& Path)
{
	nString RealAssign = Assign;
	RealAssign.ToLower();
	nString& PathString = Assigns.At(RealAssign.CStr());
	PathString = Path;
	PathString.ConvertBackslashes();
	if (PathString[PathString.Length() - 1] != '/') PathString.Add('/');
}
//---------------------------------------------------------------------

nString CIOServer::GetAssign(const nString& Assign) const
{
	nString RealAssign = Assign;
	RealAssign.ToLower();
	nString Str;
	return Assigns.Get(RealAssign.CStr(), Str) ? Str : nString::Empty;
}
//---------------------------------------------------------------------

nString CIOServer::ManglePath(const nString& Path) const
{
	nString PathString = Path;
	PathString.ConvertBackslashes();

	int ColonIdx;

	// Ignore one character "assigns" because they are really DOS drive letters
	while ((ColonIdx = PathString.FindCharIndex(':')) > 1)
	{
#ifdef _EDITOR
		if (QueryMangledPath(PathString, PathString)) continue;
#endif
		nString Assign = PathString.SubString(0, ColonIdx);
		Assign.ToLower();
		nString AssignValue;
		if (!Assigns.Get(Assign.CStr(), AssignValue)) return nString::Empty;
		PathString = AssignValue + PathString.SubString(ColonIdx + 1, PathString.Length() - (ColonIdx + 1));
	}

	PathString.StripTrailingSlash();
	return PathString;
}
//---------------------------------------------------------------------

bool CIOServer::LoadFileToBuffer(const nString& FileName, Data::CBuffer& Buffer)
{
	CFileStream File;
	if (!File.Open(FileName, SAM_READ, SAP_SEQUENTIAL)) FAIL;
	int FileSize = File.GetSize();
	Buffer.Reserve(FileSize);
	Buffer.Trim(File.Read(Buffer.GetPtr(), FileSize));
	//n_printf("FileIO: File \"%s\" successfully loaded from HDD\n", FileName.CStr());
	return Buffer.GetSize() == FileSize;
}
//---------------------------------------------------------------------

#ifdef _EDITOR
bool CIOServer::QueryMangledPath(const nString& FileName, nString& MangledFileName)
{
	if (!DataPathCB) FAIL;

	LPSTR pMangledStr = NULL;
	if (!DataPathCB(FileName.CStr(), &pMangledStr))
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
