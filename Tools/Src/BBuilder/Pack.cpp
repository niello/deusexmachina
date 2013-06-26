#include <IO/IOServer.h>
#include <IO/FS/NpkTOC.h>
#include <IO/Streams/FileStream.h>
#include <ConsoleApp.h>

bool PackFiles(const nArray<nString>& FilesToPack, const nString& PkgFileName, const nString& PkgRoot, nString PkgRootDir)
{
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
		n_msg(VR_WARNING, "File is out of the export package scope:\n - %s\n", FileName.CStr());
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
					//!!!need array shrinking method without reallocation!
					while (DirStack.GetCount() > DirCount)
					{
						TOC.EndDirEntry();
						DirStack.EraseAt(DirStack.GetCount() - 1);
					}
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

		//!!!need array shrinking method without reallocation!
		while (DirStack.GetCount() > DirCount)
		{
			TOC.EndDirEntry();
			DirStack.EraseAt(DirStack.GetCount() - 1);
		}

		nString FilePart = RelFile.ExtractFileName();
		nString FullFilePath = RootDirPath + "/" + RelFile;

		IO::CFileStream File;
		if (File.Open(FullFilePath, IO::SAM_READ))
		{
			int FileLength = File.GetSize();
			File.Close();
			TOC.AddFileEntry(FilePart.CStr(), Offset, FileLength);
			Offset += FileLength;
		}
		else
		{
			//!!!if it is a directory, add a whole directory!

			n_msg(VR_ERROR, "Error reading file %s\n", FullFilePath.CStr());
		}
	}

	TOC.EndDirEntry();

	for (; i < FilesToPack.GetCount(); ++i)
		n_msg(VR_WARNING, "File is out of the export package scope:\n - %s\n", FilesToPack[i].CStr());

	OK;
}
//---------------------------------------------------------------------
