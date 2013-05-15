#include "IOServer.h"

#include <Data/Buffer.h>
#include <IO/FS/FileSystemWin32.h>
#include <IO/FS/FileSystemNPK.h>

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
	if (DefaultFS->GetSystemFolderPath(SF_HOME, SysFolder))	SetAssign("home", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_BIN, SysFolder))	SetAssign("bin", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_USER, SysFolder))	SetAssign("user", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_TEMP, SysFolder))	SetAssign("temp", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_APP_DATA, SysFolder))	SetAssign("appdata", SysFolder);
	if (DefaultFS->GetSystemFolderPath(SF_PROGRAMS, SysFolder))	SetAssign("programs", SysFolder);
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

	//!!!mangle NPKPath and check if this NPK is already mounted!

	nString RealRoot;
	if (Root.IsValid()) RealRoot = Root;
	else RealRoot = NPKPath.ExtractDirName();

	if (!NewFS->Mount(NPKPath, RealRoot)) FAIL;
	FS.Append(NewFS);
	OK;
}
//---------------------------------------------------------------------

bool CIOServer::FileExists(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->FileExists(Path)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::IsFileReadOnly(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->FileExists(Path))
		return DefaultFS->IsFileReadOnly(Path);
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(Path))
			return FS[i]->IsFileReadOnly(Path);
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::SetFileReadOnly(const nString& Path, bool ReadOnly) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->FileExists(Path))
		return DefaultFS->SetFileReadOnly(Path, ReadOnly);
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->FileExists(Path))
			return FS[i]->SetFileReadOnly(Path, ReadOnly);
	FAIL;
}
//---------------------------------------------------------------------

#undef DeleteFile
bool CIOServer::DeleteFile(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->DeleteFile(Path)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DeleteFile(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

DWORD CIOServer::GetFileSize(const nString& Path) const
{
	//???mangle here for perf reasons?
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
	//???mangle here for perf reasons?
	if (DefaultFS->DirectoryExists(Path)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DirectoryExists(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::CreateDirectory(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->CreateDirectory(Path)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->CreateDirectory(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CIOServer::DeleteDirectory(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->DeleteDirectory(Path)) OK;
	for (int i = 0; i < FS.GetCount(); ++i)
		if (FS[i]->DeleteDirectory(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

#undef CopyFile
bool CIOServer::CopyFile(const nString& SrcPath, const nString& DestPath)
{
	//???mangle here for perf reasons?

	if (IsFileReadOnly(DestPath))
		SetFileReadOnly(DestPath, false);

	CFileStream Src, Dest;
	if (!Src.Open(SrcPath, SAM_READ, SAP_SEQUENTIAL)) FAIL;
	if (!Dest.Open(DestPath, SAM_WRITE, SAP_SEQUENTIAL))
	{
		Src.Close();
		FAIL;
	}

	int Size = Src.GetSize();
	char* pBuffer = (char*)n_malloc(Size);
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

void* CIOServer::OpenFile(PFileSystem& OutFS, const nString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern) const
{
	void* hFile = DefaultFS->OpenFile(Path, Mode, Pattern);
	if (hFile)
	{
		OutFS = DefaultFS;
		return hFile;
	}

	for (int i = 0; i < FS.GetCount(); ++i)
	{
		hFile = FS[i]->OpenFile(Path, Mode, Pattern);
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
	void* hDir = DefaultFS->OpenDirectory(Path, Filter, OutName, OutType);
	if (hDir)
	{
		OutFS = DefaultFS;
		return hDir;
	}

	for (int i = 0; i < FS.GetCount(); ++i)
	{
		hDir = FS[i]->OpenDirectory(Path, Filter, OutName, OutType);
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
	nString PathString = Path;
	PathString.StripTrailingSlash();
	PathString.Append("/");
	Assigns.At(Assign.CStr()) = PathString;
}
//---------------------------------------------------------------------

nString CIOServer::GetAssign(const nString& Assign)
{
	nString Str;
	return Assigns.Get(Assign.CStr(), Str) ? Str : nString::Empty;
}
//---------------------------------------------------------------------

nString CIOServer::ManglePath(const nString& Path)
{
	nString PathString = Path;

	int ColonIdx;
	while ((ColonIdx = PathString.FindCharIndex(':', 0)) > 0)
	{
		// Special case: ignore one character "assigns" because they are really DOS drive letters
		if (ColonIdx > 1)
		{
#ifdef _EDITOR
			if (QueryMangledPath(PathString, PathString)) continue;
#endif
			nString Assign = GetAssign(PathString.SubString(0, ColonIdx));
			if (Assign.IsEmpty()) return nString::Empty;
			Assign.Append(PathString.SubString(ColonIdx + 1, PathString.Length() - (ColonIdx + 1)));
			PathString = Assign;
		}
		else break;
	}
	PathString.ConvertBackslashes();
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
	n_printf("FileIO: File \"%s\" successfully loaded from HDD\n", FileName.CStr());
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
