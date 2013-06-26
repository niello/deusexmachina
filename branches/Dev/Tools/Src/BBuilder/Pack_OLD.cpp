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

void WriteTOCEntry(Data::CFileStream* pFile, CNpkTOCEntry* pTOCEntry)
{
	n_assert(pFile);
	n_assert(pTOCEntry);

	Data::EFSEntryType entryType = pTOCEntry->GetType();
	const char* pEntryName = pTOCEntry->GetName();
	int EntryNameLen = strlen(pEntryName);

	if (Data::FSE_DIR == entryType)
	{
		int blockLen = sizeof(short) + EntryNameLen;
		pFile->Put<int>('DIR_');
		pFile->Put<int>(blockLen);
		pFile->Put<short>(EntryNameLen);
		pFile->Write(pEntryName, EntryNameLen);

		CNpkTOCEntry* curSubEntry = pTOCEntry->GetFirstEntry();
		while (curSubEntry)
		{
			WriteTOCEntry(pFile, curSubEntry);
			curSubEntry = pTOCEntry->GetNextEntry(curSubEntry);
		}

		pFile->Put<int>('DEND');
		pFile->Put<int>(0);
	}
	else if (Data::FSE_FILE == entryType)
	{
		int blockLen = 2 * sizeof(int) + sizeof(short) + EntryNameLen;
		pFile->Put<int>('FILE');
		pFile->Put<int>(blockLen);
		pFile->Put<int>(pTOCEntry->GetFileOffset());
		pFile->Put<int>(pTOCEntry->GetFileLength());
		pFile->Put<short>(EntryNameLen);
		pFile->Write(pEntryName, EntryNameLen);
	}
}
//---------------------------------------------------------------------

bool WriteEntryData(Data::CFileStream* pFile, CNpkTOCEntry* pTOCEntry, int dataBlockOffset, int& dataLen)
{
    n_assert(pFile);
    n_assert(pTOCEntry);

	Data::EFSEntryType entryType = pTOCEntry->GetType();
    const char* pEntryName = pTOCEntry->GetName();

    if (Data::FSE_DIR == entryType)
    {
        CNpkTOCEntry* curSubEntry = pTOCEntry->GetFirstEntry();
        while (curSubEntry)
        {
            if (!WriteEntryData(pFile, curSubEntry, dataBlockOffset, dataLen))
                return false;
            curSubEntry = pTOCEntry->GetNextEntry(curSubEntry);
        }
    }
    else if (Data::FSE_FILE == entryType)
    {
        // make sure the file is still consistent with the toc data
        n_assert(pFile->GetPosition() == (dataBlockOffset + pTOCEntry->GetFileOffset()));

        // get the full source path name
        nString fileName = pTOCEntry->GetFullName();

        // read source file data
		Data::CFileStream SrcFile;
		if (SrcFile.Open(fileName, Data::SAM_READ))
        {
            // allocate buffer for file and file contents
            char* buffer = n_new_array(char, pTOCEntry->GetFileLength());
            int bytesRead = SrcFile.Read(buffer, pTOCEntry->GetFileLength());
            SrcFile.Close();
            if (bytesRead != pTOCEntry->GetFileLength())
            {
                n_delete(buffer);
                n_printf("Error reading file '%s'!\n", fileName.Get());
                return false;
            }

            // write buffer to target pFile
            int bytesWritten = pFile->Write(buffer, pTOCEntry->GetFileLength());
            n_delete(buffer);
            if (bytesWritten != pTOCEntry->GetFileLength())
            {
                n_printf("Error writing to target file!\n");
                return false;
            }
            dataLen += pTOCEntry->GetFileLength();
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
