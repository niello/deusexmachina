#include "FileSystemNPK.h"

#include <Data/StringUtils.h>
#include <IO/PathUtils.h>

namespace IO
{

bool CFileSystemNPK::Mount(const char* pSource, const char* pRoot)
{
	if (pNPKData) n_delete(pNPKData);
	pNPKData = n_new(CFileStream)(pSource);

	if (!pNPKData->Open(SAM_READ, SAP_RANDOM)) FAIL;

	TOC.SetRootPath(pRoot);

	int Value;
	pNPKData->Read(&Value, sizeof(int));
	if (Value != 'NPK0') // Check magic
	{
		n_delete(pNPKData);
		FAIL;
	}
	pNPKData->Read(&Value, sizeof(int)); // Skip int (block length)
	pNPKData->Read(&Value, sizeof(int));
	int DataOffset = Value + 8;

	char NameBuffer[DEM_MAX_PATH];

	bool InsideTOC = true;
	while (InsideTOC)
	{
		int FourCC;
		pNPKData->Read(&FourCC, sizeof(int));
		pNPKData->Read(&Value, sizeof(int)); // Skip int (block length)

		if (FourCC == 'DIR_')
		{
			short NameLen;
			pNPKData->Read(&NameLen, sizeof(short));
			n_assert(NameLen < DEM_MAX_PATH);
			pNPKData->Read(NameBuffer, NameLen);
			NameBuffer[NameLen] = 0;

			// Placeholder root directory name
			if (!strcmp(NameBuffer, "<noname>"))
			{
				CString NewName = PathUtils::ExtractFileNameWithoutExtension(pSource);
				NewName.ToLower();
				strncpy_s(NameBuffer, sizeof(NameBuffer), NewName.CStr(), _TRUNCATE);
			}

			TOC.BeginDirEntry(NameBuffer);
		}
		else if (FourCC == 'DEND') TOC.EndDirEntry();
		else if (FourCC == 'FILE')
		{
			int Offset, Len;
			pNPKData->Read(&Offset, sizeof(int));
			pNPKData->Read(&Len, sizeof(int));

			short NameLen;
			pNPKData->Read(&NameLen, sizeof(short));
			n_assert(NameLen < DEM_MAX_PATH);
			pNPKData->Read(NameBuffer, NameLen);
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
	n_delete(pNPKData);
	pNPKData = NULL;
	//???clear toc?
}
//---------------------------------------------------------------------

bool CFileSystemNPK::FileExists(const char* pPath)
{
	CNpkTOCEntry* pTE = TOC.FindEntry(pPath);
	return pTE && pTE->GetType() == FSE_FILE;
}
//---------------------------------------------------------------------

bool CFileSystemNPK::IsFileReadOnly(const char* pPath)
{
	OK; // All NPK files are readonly
}
//---------------------------------------------------------------------

bool CFileSystemNPK::DeleteFile(const char* pPath)
{
	FAIL; // Readonly FS
}
//---------------------------------------------------------------------

bool CFileSystemNPK::CopyFile(const char* pSrcPath, const char* pDestPath)
{
	FAIL; // Readonly FS
}
//---------------------------------------------------------------------

bool CFileSystemNPK::DirectoryExists(const char* pPath)
{
	CNpkTOCEntry* pTE = TOC.FindEntry(pPath);
	return pTE && pTE->GetType() == FSE_DIR;
}
//---------------------------------------------------------------------

bool CFileSystemNPK::CreateDirectory(const char* pPath)
{
	FAIL; // Readonly FS
}
//---------------------------------------------------------------------

bool CFileSystemNPK::DeleteDirectory(const char* pPath)
{
	FAIL; // Readonly FS
}
//---------------------------------------------------------------------

bool CFileSystemNPK::GetSystemFolderPath(ESystemFolder Code, CString& OutPath)
{
	// VFS doesn't provide any system directories
	OutPath.Clear();
	FAIL;
}
//---------------------------------------------------------------------

void* CFileSystemNPK::OpenDirectory(const char* pPath, const char* pFilter, CString& OutName, EFSEntryType& OutType)
{
	CNpkTOCEntry* pTE = TOC.FindEntry(pPath);
	if (pTE && pTE->GetType() == FSE_DIR)
	{
		CNPKDir* pDir = n_new(CNPKDir(pTE));
		pDir->Filter = !strcmp(pFilter, "*") ? CString::Empty : CString(pFilter);

		if (pFilter && *pFilter)
			while (pDir->It && !StringUtils::MatchesPattern(pDir->It.GetValue()->GetName(), pFilter))
				++pDir->It;

		if (pDir->It)
		{
			OutName = pDir->It.GetValue()->GetName();
			OutType = pDir->It.GetValue()->GetType() == FSE_DIR ? FSE_DIR : FSE_FILE;
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
	if (hDir) n_delete(hDir);
}
//---------------------------------------------------------------------

bool CFileSystemNPK::NextDirectoryEntry(void* hDir, CString& OutName, EFSEntryType& OutType)
{
	n_assert(hDir);
	CNPKDir* pDir = ((CNPKDir*)hDir);
	if (pDir->It)
	{
		++pDir->It;

		if (pDir->Filter.IsValid())
			while (pDir->It && !StringUtils::MatchesPattern(pDir->It.GetValue()->GetName(), pDir->Filter.CStr()))
				++pDir->It;

		if (pDir->It)
		{
			OutName = pDir->It.GetValue()->GetName();
			OutType = pDir->It.GetValue()->GetType() == FSE_DIR ? FSE_DIR : FSE_FILE;
			OK;
		}
	}

	OutName.Clear();
	OutType = FSE_NONE;
	FAIL;
}
//---------------------------------------------------------------------

void* CFileSystemNPK::OpenFile(const char* pPath, EStreamAccessMode Mode, EStreamAccessPattern /*Pattern*/)
{
	if (!pPath || !*pPath) return NULL;
	CNpkTOCEntry* pTE = TOC.FindEntry(pPath);
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

	pNPKData->Seek(pTE->GetFileOffset() + Pos, Seek_Begin);
	DWORD BytesRead = pNPKData->Read(pData, Size);
	Pos += BytesRead;
	return BytesRead;
}
//---------------------------------------------------------------------

DWORD CFileSystemNPK::Write(void* hFile, const void* pData, DWORD Size)
{
	Sys::Error("CFileSystemNPK is read only");
	return 0;
}
//---------------------------------------------------------------------

void CFileSystemNPK::Flush(void* hFile)
{
	Sys::Error("CFileSystemNPK is read only");
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