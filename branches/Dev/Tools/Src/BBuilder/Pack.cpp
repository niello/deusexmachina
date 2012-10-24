#include <Data/FS/NpkTOC.h>
#include <Data/Streams/FileStream.h>
#include <Data/FSBrowser.h>

using namespace Data;

inline const char* StripRootPath(const char* Path, const char* RootPath)
{
	n_assert(RootPath);
	size_t RootPathLen = strlen(RootPath);
	if (!strncmp(RootPath, Path, RootPathLen) && (strlen(Path) > (RootPathLen + 1)))
		return &(Path[RootPathLen + 1]);
	return NULL;
}
//---------------------------------------------------------------------

void FilterByFolder(const nString& Folder, nArray<nString>& In, nArray<nString>& Out)
{
	for (int i = In.Size() - 1; i >= 0; i--)
	{
		if (In[i].IsEmpty())
		{
			In.Erase(i);
			continue;
		}
		const char* pStripped = StripRootPath(In[i].Get(), Folder.Get());
		if (pStripped)
		{
			Out.Append(pStripped);
			In.Erase(i);
		}
	}
}
//---------------------------------------------------------------------

bool AddFileToTOC(const nString& FullEntryName, CNpkTOC& TOCObj, int& Offset)
{
	Data::CFileStream File;
	if (File.Open(FullEntryName, Data::SAM_READ))
	{
		int FileLength = File.GetSize();
		File.Close();

		nString EntryName = FullEntryName.ExtractFileName();
		EntryName.ToLower();
		n_printf("-> Adding file '%s' at %d len %d\n", EntryName.Get(), Offset, FileLength);
		TOCObj.AddFileEntry(EntryName.Get(), Offset, FileLength);
		Offset += FileLength;
	}
	else n_printf("*** ERROR: Could not open file '%s', skipping...\n", FullEntryName.Get());
	return true;
}
//---------------------------------------------------------------------

bool AddFilesToTOC(nArray<nString>& Files, CNpkTOC& TOCObj, int& Offset)
{
	// Add files that are located directly in current directory
	nString DirName = TOCObj.GetCurrentDirEntry()->GetFullName() + "/";
	for (int i = Files.Size() - 1; i >= 0; i--)
	{
		//if (Files[i].ExtractFileName() == Files[i])
		if (Files[i].FindCharIndex('/', 0) == -1)
		{
			AddFileToTOC(DirName + Files[i], TOCObj, Offset);
			Files.Erase(i);
		}
	}

	while (Files.Size() > 0)
	{
		nString DirName = Files[0].SubString(0, Files[0].FindCharIndex('/', 0));
		
		n_printf("-> Writing NPK directory '%s'\n", DirName.Get());
		
		TOCObj.BeginDirEntry(DirName.Get());
		
		nArray<nString> DirFiles;
		FilterByFolder(DirName, Files, DirFiles);
		AddFilesToTOC(DirFiles, TOCObj, Offset);
		
		n_printf("-> End directory %s\n", DirName.Get());
		
		TOCObj.EndDirEntry();
	}

	return true;
}
//---------------------------------------------------------------------

bool AddDirectoryToTOC(const nString& DirName, CNpkTOC& TOCObj, int& Offset)
{
	bool Result = true;

	nString LoweredDirName = DirName;
	LoweredDirName.StripTrailingSlash();
	LoweredDirName.ToLower();

	const nString SVN(".svn");
	if (LoweredDirName == SVN) OK;

	n_printf("-> Writing NPK directory '%s'\n", LoweredDirName.Get());

	CNpkTOCEntry* pNPKDir = TOCObj.BeginDirEntry(LoweredDirName.Get());

	CFSBrowser Browser;
	nString FullDirName = pNPKDir->GetFullName() + "/";
	if (Browser.SetAbsolutePath(FullDirName))
	{
		if (!Browser.IsCurrDirEmpty()) do
		{
			Data::EFSEntryType CurrEntryType = Browser.GetCurrEntryType();

			if (Data::FSE_FILE == CurrEntryType)
			{
				nString FullEntryName = FullDirName + Browser.GetCurrEntryName();
				FullEntryName.ToLower();
				AddFileToTOC(FullEntryName, TOCObj, Offset);
			}
			else if (Data::FSE_DIR == CurrEntryType)
			{
				if (!AddDirectoryToTOC(Browser.GetCurrEntryName(), TOCObj, Offset))
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
		n_printf("Could not open directory '%s' for reading!\n", FullDirName.Get());
		Result = false;
	}

	n_printf("-> End directory %s\n", LoweredDirName.Get());

	TOCObj.EndDirEntry();

	return Result;
}
//---------------------------------------------------------------------

void WriteTOCEntry(Data::CFileStream* pFile, CNpkTOCEntry* tocEntry)
{
	n_assert(pFile);
	n_assert(tocEntry);

	Data::EFSEntryType entryType = tocEntry->GetType();
	const char* entryName = tocEntry->GetName();
	int entryNameLen = strlen(entryName);

	if (Data::FSE_DIR == entryType)
	{
		//n_printf("=> Writing dir entry '%s'\n", entryName);

		// write a directory entry, and recurse
		int blockLen = sizeof(short) + entryNameLen;
		pFile->Put<int>('DIR_');
		pFile->Put<int>(blockLen);
		pFile->Put<short>(entryNameLen);
		pFile->Write(entryName, entryNameLen);

		CNpkTOCEntry* curSubEntry = tocEntry->GetFirstEntry();
		while (curSubEntry)
		{
			WriteTOCEntry(pFile, curSubEntry);
			curSubEntry = tocEntry->GetNextEntry(curSubEntry);
		}

		// write a final directory end marker
		pFile->Put<int>('DEND');
		pFile->Put<int>(0);
	}
	else if (Data::FSE_FILE == entryType)
	{
		//n_printf("=> Writing file entry '%s', offset %d len %d\n", entryName, entryFileOffset, entryFileLength);

		// write a file entry
		int blockLen = 2 * sizeof(int) + sizeof(short) + entryNameLen;
		pFile->Put<int>('FILE');
		pFile->Put<int>(blockLen);
		pFile->Put<int>(tocEntry->GetFileOffset());
		pFile->Put<int>(tocEntry->GetFileLength());
		pFile->Put<short>(entryNameLen);
		pFile->Write(entryName, entryNameLen);
	}
}
//---------------------------------------------------------------------

bool WriteEntryData(Data::CFileStream* pFile, CNpkTOCEntry* tocEntry, int dataBlockOffset, int& dataLen)
{
    n_assert(pFile);
    n_assert(tocEntry);

	Data::EFSEntryType entryType = tocEntry->GetType();
    const char* entryName = tocEntry->GetName();

    if (Data::FSE_DIR == entryType)
    {
        //n_printf("=> enter dir '%s'\n", entryName);

        // a dir entry, just recurse
        CNpkTOCEntry* curSubEntry = tocEntry->GetFirstEntry();
        while (curSubEntry)
        {
            if (!WriteEntryData(pFile, curSubEntry, dataBlockOffset, dataLen))
                return false;
            curSubEntry = tocEntry->GetNextEntry(curSubEntry);
        }
    }
    else if (Data::FSE_FILE == entryType)
    {
        // make sure the file is still consistent with the toc data
        n_assert(pFile->GetPosition() == (dataBlockOffset + tocEntry->GetFileOffset()));

        // get the full source path name
        nString fileName = tocEntry->GetFullName();

        // read source file data
		Data::CFileStream SrcFile;
		if (SrcFile.Open(fileName, Data::SAM_READ))
        {
            // allocate buffer for file and file contents
            char* buffer = n_new_array(char, tocEntry->GetFileLength());
            int bytesRead = SrcFile.Read(buffer, tocEntry->GetFileLength());
            SrcFile.Close();
            if (bytesRead != tocEntry->GetFileLength())
            {
                n_delete(buffer);
                n_printf("Error reading file '%s'!\n", fileName.Get());
                return false;
            }

            // write buffer to target pFile
            int bytesWritten = pFile->Write(buffer, tocEntry->GetFileLength());
            n_delete(buffer);
            if (bytesWritten != tocEntry->GetFileLength())
            {
                n_printf("Error writing to target file!\n");
                return false;
            }
            dataLen += tocEntry->GetFileLength();
        }
        else
        {
            n_printf("Failed to open source file '%s'!\n", fileName.Get());
            return false;
        }
    }
    return true;
}
//---------------------------------------------------------------------

bool WriteNPK(const nString& NpkName, CNpkTOC& TOCObj)
{
	Data::CFileStream File;
	DataSrv->CreateDirectory(NpkName.ExtractDirName());
	if (File.Open(NpkName, Data::SAM_WRITE))
	{
		File.Put<int>('NPK0');       // magic number
		File.Put<int>(4);            // block len
		File.Put<int>(0);            // dataOffset (fixed later)

		n_printf("-> Writing toc...\n");
		WriteTOCEntry(&File, TOCObj.GetRootEntry());

		n_printf("-> Writing data block...\n");
		
		int dataBlockStart = File.GetPosition();
		File.Put<int>('DATA');
		int dataLenOffset = File.GetPosition();
		File.Put<int>(0);    // fix later

		int dataSize = 0;
		if (WriteEntryData(&File, TOCObj.GetRootEntry(), dataBlockStart + 8, dataSize))
		{
			File.Seek(8, Data::SSO_BEGIN);
			File.Put<int>(dataBlockStart);

			File.Seek(dataLenOffset, Data::SSO_BEGIN);
			File.Put<int>(dataSize);
			File.Seek(0, Data::SSO_END);

			n_printf("-> All done\n");
		}
		else
		{
			n_printf("*** ERROR WRITING DATA BLOCK\n");
			File.Close();
			return false;
		}

		File.Close();
	}
	else
	{
		n_printf("Could not open file '%s' for writing!\n", NpkName.Get());
		return false;
	}

	return true;
}
//---------------------------------------------------------------------
