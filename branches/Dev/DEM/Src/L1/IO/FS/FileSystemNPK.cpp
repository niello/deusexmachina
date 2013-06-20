#include "FileSystemNPK.h"

#include <IO/IOServer.h>

namespace IO
{

bool CFileSystemNPK::Mount(const nString& Source, const nString& Root)
{
	if (!NPKData.Open(Source, SAM_READ, SAP_RANDOM)) FAIL;

	nString RealRoot = IOSrv->ManglePath(Root);

	TOC.SetRootPath(RealRoot.CStr());

	int Value;
	NPKData.Read(&Value, sizeof(int));
	if (Value != 'NPK0') // Check magic
	{
		NPKData.Close();
		FAIL;
	}
	NPKData.Read(&Value, sizeof(int)); // Skip int (block len)
	NPKData.Read(&Value, sizeof(int));
	int DataOffset = Value + 8;

	char NameBuffer[N_MAXPATH];

	bool InsideTOC = true;
	while (InsideTOC)
	{
		int FourCC;
		NPKData.Read(&FourCC, sizeof(int));
		NPKData.Read(&Value, sizeof(int)); // Skip int (block len)

		if (FourCC == 'DIR_')
		{
			short NameLen;
			NPKData.Read(&NameLen, sizeof(short));
			n_assert(NameLen < N_MAXPATH);
			NPKData.Read(NameBuffer, NameLen);
			NameBuffer[NameLen] = 0;

			// Placeholder root directory name
			if (!strcmp(NameBuffer, "<noname>"))
			{
				nString NewName = Source.ExtractFileName();
				NewName.StripExtension();
				NewName.ToLower();
				n_strncpy2(NameBuffer, NewName.CStr(), sizeof(NameBuffer));
			}

			TOC.BeginDirEntry(NameBuffer);
		}
		else if (FourCC == 'DEND') TOC.EndDirEntry();
		else if (FourCC == 'FILE')
		{
			int Offset, Len;
			NPKData.Read(&Offset, sizeof(int));
			NPKData.Read(&Len, sizeof(int));

			short NameLen;
			NPKData.Read(&NameLen, sizeof(short));
			n_assert(NameLen < N_MAXPATH);
			NPKData.Read(NameBuffer, NameLen);
			NameBuffer[NameLen] = 0;

			Offset += DataOffset;
			TOC.AddFileEntry(NameBuffer, Offset, Len);
		}
		else InsideTOC = false;
	}

	OK;
}
//---------------------------------------------------------------------

void CFileSystemNPK::Unmount()
{
	NPKData.Close();
}
//---------------------------------------------------------------------

bool CFileSystemNPK::FileExists(const nString& Path)
{
	CNpkTOCEntry* pTE = TOC.FindEntry(IOSrv->ManglePath(Path).CStr());
	return pTE && pTE->GetType() == FSE_FILE;
}
//---------------------------------------------------------------------

bool CFileSystemNPK::IsFileReadOnly(const nString& Path)
{
	OK; // All NPK files are readonly
}
//---------------------------------------------------------------------

bool CFileSystemNPK::DeleteFile(const nString& Path)
{
	FAIL; // Readonly FS
}
//---------------------------------------------------------------------

bool CFileSystemNPK::CopyFile(const nString& SrcPath, const nString& DestPath)
{
	FAIL; // Readonly FS
}
//---------------------------------------------------------------------

bool CFileSystemNPK::DirectoryExists(const nString& Path)
{
	CNpkTOCEntry* pTE = TOC.FindEntry(IOSrv->ManglePath(Path).CStr());
	return pTE && pTE->GetType() == FSE_DIR;
}
//---------------------------------------------------------------------

bool CFileSystemNPK::CreateDirectory(const nString& Path)
{
	FAIL; // Readonly FS
}
//---------------------------------------------------------------------

bool CFileSystemNPK::DeleteDirectory(const nString& Path)
{
	FAIL; // Readonly FS
}
//---------------------------------------------------------------------

bool CFileSystemNPK::GetSystemFolderPath(ESystemFolder Code, nString& OutPath)
{
	// VFS doesn't provide any system directories
	OutPath.Clear();
	FAIL;
}
//---------------------------------------------------------------------

void* CFileSystemNPK::OpenDirectory(const nString& Path, const nString& Filter, nString& OutName, EFSEntryType& OutType)
{
	CNpkTOCEntry* pTE = TOC.FindEntry(IOSrv->ManglePath(Path).CStr());
	if (pTE && pTE->GetType() == FSE_DIR)
	{
		CNPKDir* pDir = n_new(CNPKDir);
		pDir->Filter = (Filter == "*") ? nString::Empty : Filter;
		pDir->pTOCEntry = pTE;
		pDir->pCurrEntry = pTE->GetFirstEntry();

		if (Filter.IsValid())
			while (pDir->pCurrEntry && !nString(pDir->pCurrEntry->GetName()).MatchPattern(Filter))
				pDir->pCurrEntry = pTE->GetNextEntry(pDir->pCurrEntry);

		if (pDir->pCurrEntry)
		{
			OutName = pDir->pCurrEntry->GetName();
			OutType = pDir->pCurrEntry->GetType() == FSE_DIR ? FSE_DIR : FSE_FILE;
		}
		else
		{
			OutName.Clear();
			OutType = FSE_NONE;
		}
		return pDir;
	}

	OutName.Clear();
	OutType = FSE_NONE;
	return NULL;
}
//---------------------------------------------------------------------

void CFileSystemNPK::CloseDirectory(void* hDir)
{
	n_assert(hDir);
	n_delete(hDir);
}
//---------------------------------------------------------------------

bool CFileSystemNPK::NextDirectoryEntry(void* hDir, nString& OutName, EFSEntryType& OutType)
{
	n_assert(hDir);
	CNPKDir* pDir = ((CNPKDir*)hDir);
	if (pDir->pCurrEntry)
	{
		pDir->pCurrEntry = pDir->pTOCEntry->GetNextEntry(pDir->pCurrEntry);

		if (pDir->Filter.IsValid())
			while (pDir->pCurrEntry && !nString(pDir->pCurrEntry->GetName()).MatchPattern(pDir->Filter))
				pDir->pCurrEntry = pDir->pTOCEntry->GetNextEntry(pDir->pCurrEntry);

		if (pDir->pCurrEntry)
		{
			OutName = pDir->pCurrEntry->GetName(); //???GetFullName()?
			OutType = pDir->pCurrEntry->GetType() == FSE_DIR ? FSE_DIR : FSE_FILE;
			OK;
		}
	}

	OutName.Clear();
	OutType = FSE_NONE;
	FAIL;
}
//---------------------------------------------------------------------

void* CFileSystemNPK::OpenFile(const nString& Path, EStreamAccessMode Mode, EStreamAccessPattern /*Pattern*/)
{
	n_assert(Path.IsValid());
	CNpkTOCEntry* pTE = TOC.FindEntry(IOSrv->ManglePath(Path).CStr());
	if (pTE && pTE->GetType() == FSE_FILE)
	{
		CNPKFile* pFile = n_new(CNPKFile);
		pFile->pTOCEntry = pTE;
		pFile->Offset = 0;
		return pFile;
	}
	return NULL;
}
//---------------------------------------------------------------------

void CFileSystemNPK::CloseFile(void* hFile)
{
	n_assert(hFile);
	n_delete(hFile);
}
//---------------------------------------------------------------------

DWORD CFileSystemNPK::Read(void* hFile, void* pData, DWORD Size)
{
	n_assert(hFile && pData && Size > 0);

	CNpkTOCEntry* pTE = ((CNPKFile*)hFile)->pTOCEntry;
	DWORD Len = pTE->GetFileLength();
	DWORD& Pos = ((CNPKFile*)hFile)->Offset;
	DWORD EndPos = Pos + Size;
	if (EndPos >= Len) Size = Len - Pos;

	NPKData.Seek(pTE->GetFileOffset() + Pos, Seek_Begin);
	DWORD BytesRead = NPKData.Read(pData, Size);
	Pos += BytesRead;
	return BytesRead;
}
//---------------------------------------------------------------------

DWORD CFileSystemNPK::Write(void* hFile, const void* pData, DWORD Size)
{
	n_error("CFileSystemNPK is read only");
	return 0;
}
//---------------------------------------------------------------------

void CFileSystemNPK::Flush(void* hFile)
{
	n_error("CFileSystemNPK is read only");
}
//---------------------------------------------------------------------

DWORD CFileSystemNPK::GetFileSize(void* hFile) const
{
	n_assert(hFile);
	return ((CNPKFile*)hFile)->pTOCEntry->GetFileLength();
}
//---------------------------------------------------------------------

bool CFileSystemNPK::Seek(void* hFile, int Offset, ESeekOrigin Origin)
{
	n_assert(hFile);
	int SeekPos;
	int Len = ((CNPKFile*)hFile)->pTOCEntry->GetFileLength();
	switch (Origin)
	{
		case Seek_Current:	SeekPos = ((CNPKFile*)hFile)->Offset + Offset; break;
		case Seek_End:		SeekPos = Len + Offset; break;
		default:			SeekPos = Offset; break;
	}

	if (SeekPos >= Len)
	{
		((CNPKFile*)hFile)->Offset = Len;
		FAIL;
	}
	else
	{
		((CNPKFile*)hFile)->Offset = n_max(SeekPos, 0);
		OK;
	}
}
//---------------------------------------------------------------------

DWORD CFileSystemNPK::Tell(void* hFile) const
{
	n_assert(hFile);
	return ((CNPKFile*)hFile)->Offset;
}
//---------------------------------------------------------------------

bool CFileSystemNPK::IsEOF(void* hFile) const
{
	n_assert(hFile);
	return ((CNPKFile*)hFile)->Offset >= (DWORD)((CNPKFile*)hFile)->pTOCEntry->GetFileLength();
}
//---------------------------------------------------------------------

}