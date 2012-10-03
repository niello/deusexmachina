#include "DataServer.h"

#include "Params.h"
#include "DataArray.h"
#include "HRDParser.h"
#include "HRDWriter.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "XMLDocument.h"
#include <Data/FS/FileSystemWin32.h>
#include <Data/FS/FileSystemNPK.h>
#include <kernel/nkernelserver.h>

namespace Data
{
ImplementRTTI(Data::CDataServer, Core::CRefCounted);

CDataServer* CDataServer::Singleton = NULL;

CDataServer::CDataServer(): HRDCache(PParams()), Assigns(nString())
{
	n_assert(!Singleton);
	Singleton = this;

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

CDataServer::~CDataServer()
{
	n_assert(Singleton);
	Singleton = NULL;
}
//---------------------------------------------------------------------

bool CDataServer::MountNPK(const nString& NPKPath, const nString& Root)
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

bool CDataServer::FileExists(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->FileExists(Path)) OK;
	for (int i = 0; i < FS.Size(); ++i)
		if (FS[i]->FileExists(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CDataServer::IsFileReadOnly(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->FileExists(Path))
		return DefaultFS->IsFileReadOnly(Path);
	for (int i = 0; i < FS.Size(); ++i)
		if (FS[i]->FileExists(Path))
			return FS[i]->IsFileReadOnly(Path);
	FAIL;
}
//---------------------------------------------------------------------

bool CDataServer::SetFileReadOnly(const nString& Path, bool ReadOnly) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->FileExists(Path))
		return DefaultFS->SetFileReadOnly(Path, ReadOnly);
	for (int i = 0; i < FS.Size(); ++i)
		if (FS[i]->FileExists(Path))
			return FS[i]->SetFileReadOnly(Path, ReadOnly);
	FAIL;
}
//---------------------------------------------------------------------

#undef DeleteFile
bool CDataServer::DeleteFile(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->DeleteFile(Path)) OK;
	for (int i = 0; i < FS.Size(); ++i)
		if (FS[i]->DeleteFile(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

DWORD CDataServer::GetFileSize(const nString& Path) const
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

bool CDataServer::DirectoryExists(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->DirectoryExists(Path)) OK;
	for (int i = 0; i < FS.Size(); ++i)
		if (FS[i]->DirectoryExists(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CDataServer::CreateDirectory(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->CreateDirectory(Path)) OK;
	for (int i = 0; i < FS.Size(); ++i)
		if (FS[i]->CreateDirectory(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CDataServer::DeleteDirectory(const nString& Path) const
{
	//???mangle here for perf reasons?
	if (DefaultFS->DeleteDirectory(Path)) OK;
	for (int i = 0; i < FS.Size(); ++i)
		if (FS[i]->DeleteDirectory(Path)) OK;
	FAIL;
}
//---------------------------------------------------------------------

#undef CopyFile
bool CDataServer::CopyFile(const nString& SrcPath, const nString& DestPath)
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

void* CDataServer::OpenFile(PFileSystem& OutFS, const nString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern) const
{
	void* hFile = DefaultFS->OpenFile(Path, Mode, Pattern);
	if (hFile)
	{
		OutFS = DefaultFS;
		return hFile;
	}

	for (int i = 0; i < FS.Size(); ++i)
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

void* CDataServer::OpenDirectory(const nString& Path, const nString& Filter,
								 PFileSystem& OutFS, nString& OutName, EFSEntryType& OutType) const
{
	void* hDir = DefaultFS->OpenDirectory(Path, Filter, OutName, OutType);
	if (hDir)
	{
		OutFS = DefaultFS;
		return hDir;
	}

	for (int i = 0; i < FS.Size(); ++i)
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

void CDataServer::SetAssign(const nString& Assign, const nString& Path)
{
	nString PathString = Path;
	PathString.StripTrailingSlash();
	PathString.Append("/");
	Assigns.At(Assign.Get()) = PathString;
}
//---------------------------------------------------------------------

nString CDataServer::GetAssign(const nString& Assign)
{
	nString Str;
	return Assigns.Get(Assign.Get(), Str) ? Str : nString::Empty;
}
//---------------------------------------------------------------------

nString CDataServer::ManglePath(const nString& Path)
{
	nString PathString = Path;

	int ColonIdx;
	while ((ColonIdx = PathString.FindCharIndex(':', 0)) > 0)
	{
		// Special case: ignore one character "assigns" because they are really DOS drive letters
		if (ColonIdx > 1)
		{
			nString Assign = GetAssign(PathString.ExtractRange(0, ColonIdx));
			if (!Assign.IsEmpty())
				Assign.Append(PathString.ExtractRange(ColonIdx + 1, PathString.Length() - (ColonIdx + 1)));
			PathString = Assign;
		}
		else break;
	}
	PathString.ConvertBackslashes();
	PathString.StripTrailingSlash();
	return PathString;
}
//---------------------------------------------------------------------

DWORD CDataServer::LoadFileToBuffer(const nString& FileName, char*& Buffer)
{
	CFileStream File;

	int BytesRead;

	if (File.Open(FileName, SAM_READ, SAP_SEQUENTIAL))
	{
		int FileSize = File.GetSize();
		Buffer = (char*)n_malloc(FileSize);
		BytesRead = File.Read(Buffer, FileSize);
		File.Close();
	}
	else
	{
		Buffer = NULL;
		BytesRead = 0;
	}

	return BytesRead;
}
//---------------------------------------------------------------------

PParams CDataServer::LoadHRD(const nString& FileName, bool Cache)
{
	PParams P;
	if (HRDCache.Get(FileName.Get(), P)) return P;
	else return ReloadHRD(FileName, Cache);
}
//---------------------------------------------------------------------

PParams CDataServer::ReloadHRD(const nString& FileName, bool Cache)
{
	char* Buffer;
	int BytesRead = LoadFileToBuffer(FileName, Buffer);

	PParams Params;
	if (!pHRDParser.isvalid()) pHRDParser = n_new(CHRDParser);
	if (pHRDParser->ParseBuffer(Buffer, BytesRead, Params))
	{
		n_printf("FileIO: HRD \"%s\" successfully loaded from HDD\n", FileName.Get());
		if (Cache) HRDCache.Add(FileName.Get(), Params); //!!!???mangle/unmangle path to avoid duplicates?
	}
	else n_printf("FileIO: HRD parsing of \"%s\" failed\n", FileName.Get());

	n_delete_array(Buffer);

	return Params;
}
//---------------------------------------------------------------------

//???remove from here? make user use readers/writers directly?
void CDataServer::SaveHRD(const nString& FileName, PParams Content)
{
	if (!Content.isvalid()) return;

	CFileStream File;
	if (!File.Open(FileName, SAM_WRITE)) return;
	CHRDWriter Writer(File);
	Writer.WriteParams(Content);
}
//---------------------------------------------------------------------

void CDataServer::UnloadHRD(const nString& FileName)
{
	HRDCache.Remove(FileName.Get());
}
//---------------------------------------------------------------------

PParams CDataServer::LoadPRM(const nString& FileName, bool Cache)
{
	PParams P;
	if (HRDCache.Get(FileName.Get(), P)) return P;
	else return ReloadPRM(FileName, Cache);
}
//---------------------------------------------------------------------

PParams CDataServer::ReloadPRM(const nString& FileName, bool Cache)
{
	CFileStream File;
	if (!File.Open(FileName, SAM_READ)) return NULL;
	CBinaryReader Reader(File);

	PParams Params;
	Params.Create();
	if (Reader.ReadParams(*Params))
	{
		if (Cache) HRDCache.Add(FileName.Get(), Params); //!!!???mangle path to avoid duplicates?
		n_printf("FileIO: PRM \"%s\" successfully loaded from HDD\n", FileName.Get());
	}
	else
	{
		Params = NULL;
		n_printf("FileIO: PRM loading from \"%s\" failed\n", FileName.Get());
	}

	return Params;
}
//---------------------------------------------------------------------

//???remove from here? make user use readers/writers directly?
void CDataServer::SavePRM(const nString& FileName, PParams Content)
{
	if (!Content.isvalid()) return;

	CFileStream File;
	if (!File.Open(FileName, SAM_WRITE)) return;
	CBinaryWriter Writer(File);
	Writer.WriteParams(*Content);
}
//---------------------------------------------------------------------

PXMLDocument CDataServer::LoadXML(const nString& FileName) //, bool Cache)
{
	char* Buffer;
	int BytesRead = DataSrv->LoadFileToBuffer(FileName, Buffer);

	PXMLDocument XML = n_new(CXMLDocument);
	if (XML->Parse(Buffer, BytesRead) == tinyxml2::XML_SUCCESS)
	{
		n_printf("FileIO: XML \"%s\" successfully loaded from HDD\n", FileName.Get());
		//if (Cache) XMLCache.Add(FileName.Get(), XML); //!!!???mangle/unmangle path to avoid duplicates?
	}
	else
	{
		n_printf("FileIO: XML parsing of \"%s\" failed: %s. %s.\n", FileName.Get(), XML->GetErrorStr1(), XML->GetErrorStr2());
		XML = NULL;
	}

	n_delete_array(Buffer);

	return XML;
}
//---------------------------------------------------------------------

//!!!need desc cache! (independent from HRD cache)
bool CDataServer::LoadDesc(PParams& Out, const nString& FileName, bool Cache)
{
	PParams Main = LoadPRM(FileName, Cache);

	if (!Main.isvalid()) FAIL;

	nString BaseName;
	if (Main->Get(BaseName, CStrID("_Base_")))
	{
		BaseName = "actors:" + BaseName + ".prm";
		n_assert(BaseName != FileName);
		if (!LoadDesc(Out, BaseName, Cache)) FAIL;
		Out->Merge(*Main, Merge_AddNew | Merge_Replace | Merge_Deep); //!!!can specify merge flags in Desc!
	}
	else Out = n_new(CParams(*Main));

	OK;
}
//---------------------------------------------------------------------

bool CDataServer::LoadDataSchemes(const nString& FileName)
{
	PParams SchemeDescs = LoadHRD(FileName, false);
	for (int i = 0; i < SchemeDescs->GetCount(); ++i)
	{
		const CParam& Prm = SchemeDescs->Get(i);
		if (!Prm.IsA<PParams>()) FAIL;

		int Idx = DataSchemes.FindIndex(Prm.GetName());
		if (Idx != INVALID_INDEX) DataSchemes.EraseAt(Idx);

		PDataScheme Scheme = n_new(CDataScheme);
		if (!Scheme->Init(*Prm.GetValue<PParams>())) FAIL;
		DataSchemes.Add(Prm.GetName(), Scheme);
	}
	OK;
}
//---------------------------------------------------------------------

} //namespace Data
