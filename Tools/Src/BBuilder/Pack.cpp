#include <IO/IOServer.h>
#include <IO/FS/NpkTOC.h>
#include <IO/FSBrowser.h>
#include <IO/Streams/FileStream.h>
#include <Data/Buffer.h>
#include <ConsoleApp.h>

void PrintNpkTOCEntry(IO::CNpkTOCEntry& Entry, int Level)
{
	nString Str;
	for (int i = 0; i < Level; ++i) Str.Append("  "); //Str.Append("| ");
	Str.Append((Entry.GetType() == IO::FSE_DIR) ? "+ " : "  ");
	Str += Entry.GetName();

	n_printf(Str.CStr());

	if (Entry.GetType() == IO::FSE_DIR)
	{
		n_printf("\n");

		IO::CNpkTOCEntry* pSubEntry = Entry.GetFirstEntry();
		while (pSubEntry)
		{
			if (pSubEntry->GetType() == IO::FSE_DIR)
				PrintNpkTOCEntry(*pSubEntry, Level + 1);
			pSubEntry = Entry.GetNextEntry(pSubEntry);
		}

		pSubEntry = Entry.GetFirstEntry();
		while (pSubEntry)
		{
			if (pSubEntry->GetType() == IO::FSE_FILE)
				PrintNpkTOCEntry(*pSubEntry, Level + 1);
			pSubEntry = Entry.GetNextEntry(pSubEntry);
		}
	}
	else n_printf(" (%d B)\n", Entry.GetFileLength());
}
//---------------------------------------------------------------------

bool AddDirectoryToTOC(nString DirName, IO::CNpkTOC& TOC, int& Offset)
{
	bool Result = true;

	DirName.StripTrailingSlash();
	DirName.ToLower();

	if (DirName == ".svn") OK;

	IO::CNpkTOCEntry* pNPKDir = TOC.BeginDirEntry(DirName.CStr());

	IO::CFSBrowser Browser;
	nString FullDirName = pNPKDir->GetFullName() + "/";
	if (Browser.SetAbsolutePath(FullDirName))
	{
		if (!Browser.IsCurrDirEmpty()) do
		{
			IO::EFSEntryType CurrEntryType = Browser.GetCurrEntryType();

			if (CurrEntryType == IO::FSE_FILE)
			{
				nString FilePart = Browser.GetCurrEntryName();
				nString FullFilePath = FullDirName + Browser.GetCurrEntryName();
				FilePart.ToLower();

				IO::CFileStream File;
				if (File.Open(FullFilePath, IO::SAM_READ))
				{
					int FileLength = File.GetSize();
					File.Close();
					TOC.AddFileEntry(FilePart.CStr(), Offset, FileLength);
					Offset += FileLength;
				}
				else n_msg(VL_ERROR, "Error reading file %s\n", FullFilePath.CStr());
			}
			else if (CurrEntryType == IO::FSE_DIR)
			{
				if (!AddDirectoryToTOC(Browser.GetCurrEntryName(), TOC, Offset))
				{
					Result = false;
					break;
				}
			}
		}
		while (Browser.NextCurrDirEntry());
	}
	else
	{
		n_msg(VL_ERROR, "Could not open directory '%s' for reading!\n", FullDirName.CStr());
		Result = false;
	}

	TOC.EndDirEntry();

	return Result;
}
//---------------------------------------------------------------------

void WriteTOCEntry(IO::CFileStream& File, IO::CNpkTOCEntry* pTOCEntry)
{
	n_assert(pTOCEntry);

	if (pTOCEntry->GetType() == IO::FSE_DIR)
	{
		File.Put<int>('DIR_');
		File.Put<int>(sizeof(short) + pTOCEntry->GetNameLength());
		File.Put<short>(pTOCEntry->GetNameLength());
		File.Write(pTOCEntry->GetName(), pTOCEntry->GetNameLength());

		IO::CNpkTOCEntry* pSubEntry = pTOCEntry->GetFirstEntry();
		while (pSubEntry)
		{
			WriteTOCEntry(File, pSubEntry);
			pSubEntry = pTOCEntry->GetNextEntry(pSubEntry);
		}

		File.Put<int>('DEND');
		File.Put<int>(0);
	}
	else if (pTOCEntry->GetType() == IO::FSE_FILE)
	{
		File.Put<int>('FILE');
		File.Put<int>(2 * sizeof(int) + sizeof(short) + pTOCEntry->GetNameLength());
		File.Put<int>(pTOCEntry->GetFileOffset());
		File.Put<int>(pTOCEntry->GetFileLength());
		File.Put<short>(pTOCEntry->GetNameLength());
		File.Write(pTOCEntry->GetName(), pTOCEntry->GetNameLength());
	}
}
//---------------------------------------------------------------------

bool WriteEntryData(IO::CFileStream& File, IO::CNpkTOCEntry* pTOCEntry, int DataOffset, int& DataSize)
{
	n_assert(pTOCEntry);

	if (pTOCEntry->GetType() == IO::FSE_DIR)
	{
		IO::CNpkTOCEntry* pSubEntry = pTOCEntry->GetFirstEntry();
		while (pSubEntry)
		{
			if (!WriteEntryData(File, pSubEntry, DataOffset, DataSize)) FAIL;
			pSubEntry = pTOCEntry->GetNextEntry(pSubEntry);
		}
	}
	else if (pTOCEntry->GetType() == IO::FSE_FILE)
	{
		n_assert(File.GetPosition() == (DataOffset + pTOCEntry->GetFileOffset()));

		nString FullFileName = pTOCEntry->GetFullName();

		//???write streamed file copying?
		Data::CBuffer Buffer;
		if (IOSrv->LoadFileToBuffer(FullFileName, Buffer))
		{
			if (File.Write(Buffer.GetPtr(), Buffer.GetSize()) != pTOCEntry->GetFileLength())
			{
				n_printf("Error writing %s to NPK!\n", FullFileName.CStr());
				FAIL;
			}
			DataSize += pTOCEntry->GetFileLength();
		}
		else
		{
			n_msg(VL_ERROR, "Error reading file %s\n", FullFileName.CStr());
			FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool PackFiles(const nArray<nString>& FilesToPack, const nString& PkgFileName, const nString& PkgRoot, nString PkgRootDir)
{
	// Create TOC

	n_msg(VL_INFO, "Creating NPK TOC...\n");

	nString RootPath = IOSrv->ManglePath(PkgRoot);
	RootPath.ToLower();

	PkgRootDir.ToLower();

	nString RootDirPath = RootPath + "/" + PkgRootDir;

	int i = 0;
	for (; i < FilesToPack.GetCount(); ++i)
	{
		const nString& FileName = FilesToPack[i];
		if (FileName.Length() <= RootDirPath.Length() + 1) continue;
		if (strncmp(RootDirPath.CStr(), FileName.CStr(), RootDirPath.Length()) >= 0) break;
		n_msg(VL_WARNING, "File is out of the export package scope:\n - %s\n", FileName.CStr());
	}

	nArray<nString> DirStack;

	int Offset = 0;

	IO::CNpkTOC TOC;
	TOC.SetRootPath(RootPath.CStr());
	TOC.BeginDirEntry(PkgRootDir.CStr());

	for (; i < FilesToPack.GetCount(); ++i)
	{
		const nString& FileName = FilesToPack[i];
		if (FileName.Length() <= RootDirPath.Length() + 1) continue;
		if (strncmp(RootDirPath.CStr(), FileName.CStr(), RootDirPath.Length()) > 0) break;

		nString RelFile = FileName.CStr() + RootDirPath.Length() + 1;

		int CurrStartChar = 0;
		int DirCount = 0;
		while (CurrStartChar < RelFile.Length())
		{
			int DirSepChar = RelFile.FindCharIndex('/', CurrStartChar);
			if (DirSepChar == INVALID_INDEX) break;

			nString Dir = RelFile.SubString(CurrStartChar, DirSepChar - CurrStartChar);
			if (DirCount < DirStack.GetCount())
			{
				if (Dir != DirStack[DirCount])
				{
					for (int StackIdx = DirCount; StackIdx < DirStack.GetCount(); ++StackIdx)
						TOC.EndDirEntry();
					DirStack.Truncate(DirStack.GetCount() - DirCount);
					TOC.BeginDirEntry(Dir.CStr());
					DirStack.Append(Dir);
				}
			}
			else
			{
				TOC.BeginDirEntry(Dir.CStr());
				DirStack.Append(Dir);
			}

			++DirCount;

			CurrStartChar = DirSepChar + 1;
		}

		for (int StackIdx = DirCount; StackIdx < DirStack.GetCount(); ++StackIdx)
			TOC.EndDirEntry();
		DirStack.Truncate(DirStack.GetCount() - DirCount);

		nString FilePart = RelFile.ExtractFileName();
		nString FullFilePath = RootDirPath + "/" + RelFile;

		if (IOSrv->DirectoryExists(FullFilePath))
		{
			if (!AddDirectoryToTOC(FilePart, TOC, Offset))
				n_msg(VL_ERROR, "Error reading directory %s\n", FullFilePath.CStr());
		}
		else
		{
			IO::CFileStream File;
			if (File.Open(FullFilePath, IO::SAM_READ))
			{
				int FileLength = File.GetSize();
				File.Close();
				TOC.AddFileEntry(FilePart.CStr(), Offset, FileLength);
				Offset += FileLength;
			}
			else n_msg(VL_ERROR, "Error reading file %s\n", FullFilePath.CStr());
		}
	}

	TOC.EndDirEntry();

	for (; i < FilesToPack.GetCount(); ++i)
		n_msg(VL_WARNING, "File is out of the export package scope:\n - %s\n", FilesToPack[i].CStr());

	// Write NPK file to disk

	n_msg(VL_INFO, "Writing NPK...\n");

	IO::CFileStream File;
	IOSrv->CreateDirectory(PkgFileName.ExtractDirName());
	if (!File.Open(PkgFileName, IO::SAM_WRITE))
	{
		n_msg(VL_ERROR, "Could not open file '%s' for writing!\n", PkgFileName.CStr());
		FAIL;
	}

	File.Put<int>('NPK0');	// Magic
	File.Put<int>(4);		// Block length
	File.Put<int>(0);		// DataBlockStart (at 8, fixed later)

	n_msg(VL_DETAILS, " - Writing TOC...\n");

	WriteTOCEntry(File, TOC.GetRootEntry());

	n_msg(VL_DETAILS, " - Writing data...\n");
	
	int DataBlockStart = File.GetPosition();
	int DataOffset = DataBlockStart + 4;

	File.Put<int>('DATA');
	File.Put<int>(0);		// DataSize (at DataOffset, fixed later)

	int DataSize = 0;
	if (WriteEntryData(File, TOC.GetRootEntry(), DataOffset + 4, DataSize))
	{
		File.Seek(8, IO::Seek_Begin);
		File.Put<int>(DataBlockStart);
		File.Seek(DataOffset, IO::Seek_Begin);
		File.Put<int>(DataSize);
		File.Seek(0, IO::Seek_End);
	}
	else
	{
		n_msg(VL_ERROR, "ERROR WRITING DATA BLOCK\n");
		FAIL;
	}

	n_msg(VL_DETAILS, " - NPK writing done\n");

	// Print TOC

	if (TOC.GetRootEntry() && Verbose >= VL_DETAILS)
	{
		n_printf("\n"SEP_LINE"Package TOC:\n'+' = directory\n"SEP_LINE);
		PrintNpkTOCEntry(*TOC.GetRootEntry(), 0);
	}

	OK;
}
//---------------------------------------------------------------------
