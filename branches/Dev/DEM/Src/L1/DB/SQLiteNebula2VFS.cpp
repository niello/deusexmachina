#include <Data/DataServer.h>
#include <Data/Streams/FileStream.h>
#include <sqlite3.h>

//!!!move functions used to FS wrapper!
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef DeleteFile
#undef CreateDirectory

using namespace Data;

struct CN2File
{
	sqlite3_file	Base;
	CFileStream*	pFile;
};

static int N2Close(sqlite3_file *pFile)
{
	CN2File* pN2File = (CN2File*)pFile;
	pN2File->pFile->Close();
	n_delete(pN2File->pFile);
	pN2File->pFile = NULL;
	return SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2Read(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite_int64 iOfst)
{
	CN2File* pN2File = (CN2File*)pFile;
	pN2File->pFile->Seek((int)iOfst, SSO_BEGIN); //???pre-check with Tell?
	return (pN2File->pFile->Read(zBuf, iAmt) == iAmt) ? SQLITE_OK : SQLITE_IOERR_WRITE;
}
//---------------------------------------------------------------------

static int N2Write(sqlite3_file *pFile, const void *zBuf, int iAmt, sqlite_int64 iOfst)
{
	CN2File* pN2File = (CN2File*)pFile;
	pN2File->pFile->Seek((int)iOfst, SSO_BEGIN); //???pre-check with Tell?
	return (pN2File->pFile->Write(zBuf, iAmt) == iAmt) ? SQLITE_OK : SQLITE_IOERR_WRITE;
}
//---------------------------------------------------------------------

static int N2Truncate(sqlite3_file *pFile, sqlite_int64 size)
{
	//!!!add SetSize/SetFileEnd to CFileStream!
	return SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2Sync(sqlite3_file *pFile, int Flags)
{
	CN2File* pN2File = (CN2File*)pFile;
	pN2File->pFile->Flush();
	return SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2FileSize(sqlite3_file *pFile, sqlite_int64 *pSize)
{
	*pSize = ((CN2File*)pFile)->pFile->GetSize();
	return SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2Lock(sqlite3_file *pFile, int eLock)
{
	return SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2Unlock(sqlite3_file *pFile, int eLock)
{
	return SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2CheckReservedLock(sqlite3_file *pFile, int *pResOut)
{
	*pResOut = 0;
	return SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2FileControl(sqlite3_file *pFile, int op, void *pArg)
{
	return (op == SQLITE_FCNTL_PRAGMA) ? SQLITE_NOTFOUND : SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2SectorSize(sqlite3_file *pFile)
{
	return 0;
}
//---------------------------------------------------------------------

static int N2DeviceCharacteristics(sqlite3_file *pFile)
{
	return SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN;
}
//---------------------------------------------------------------------

static const sqlite3_io_methods N2SQLiteIO =
{
	1,							/* iVersion */
	N2Close,					/* xClose */
	N2Read,						/* xRead */
	N2Write,					/* xWrite */
	N2Truncate,					/* xTruncate */
	N2Sync,						/* xSync */
	N2FileSize,					/* xFileSize */
	N2Lock,						/* xLock */
	N2Unlock,					/* xUnlock */
	N2CheckReservedLock,		/* xCheckReservedLock */
	N2FileControl,				/* xFileControl */
	N2SectorSize,				/* xSectorSize */
	N2DeviceCharacteristics		/* xDeviceCharacteristics */
};
//=====================================================================

static int N2Open(sqlite3_vfs *pVfs, const char *zName, sqlite3_file *pFile, int Flags, int *pOutFlags)
{
	if (!zName) return SQLITE_IOERR; // No tmp files for now

	const nString FileName = zName;

	CFileStream* pN2File = n_new(CFileStream);
	
	bool IsReadOnly;

	bool FileExists = DataSrv->FileExists(FileName);
	if (!FileExists)
	{
		if (Flags & SQLITE_OPEN_READONLY ||
			!(Flags & SQLITE_OPEN_CREATE) ||
			!DataSrv->CreateDirectory(FileName.ExtractDirName()))
		{
			n_delete(pN2File);
			return SQLITE_CANTOPEN;
		}
		IsReadOnly = false;
	}
	else IsReadOnly = DataSrv->IsFileReadOnly(FileName);

	EStreamAccessMode AccessMode;
	if (Flags & SQLITE_OPEN_READWRITE)
	{
		if (IsReadOnly)
		{
			Flags &= ~SQLITE_OPEN_READWRITE;
			Flags |= SQLITE_OPEN_READONLY;
			AccessMode = SAM_READ;
		}
		else AccessMode = SAM_READWRITE;
	}
	else AccessMode = SAM_READ;

	if (!pN2File->Open(FileName, AccessMode, SAP_RANDOM)) return SQLITE_CANTOPEN;

	pFile->pMethods = &N2SQLiteIO;
	((CN2File*)pFile)->pFile = pN2File;

	if (pOutFlags) *pOutFlags = Flags;

	return SQLITE_OK;
}
//---------------------------------------------------------------------

static int N2Delete(sqlite3_vfs *pVfs, const char *zPath, int dirSync)
{
	return DataSrv->DeleteFile(zPath) ? SQLITE_OK : SQLITE_IOERR_DELETE;
}
//---------------------------------------------------------------------

static int N2Access(sqlite3_vfs *pVfs, const char *zPath, int Flags, int *pResOut)
{
	n_assert(Flags == SQLITE_ACCESS_EXISTS || Flags == SQLITE_ACCESS_READ || Flags == SQLITE_ACCESS_READWRITE);
	n_assert(pResOut);

	const nString FileName = zPath;

	bool FileExists = DataSrv->FileExists(FileName);

	if (Flags == SQLITE_ACCESS_EXISTS)
	{
		*pResOut = FileExists;
		return SQLITE_OK;
	}
	else if (FileExists)
	{
		if (Flags == SQLITE_ACCESS_READ)
		{
			*pResOut = true;
			return SQLITE_OK;
		}
		else if (Flags == SQLITE_ACCESS_READWRITE)
		{
			*pResOut = !DataSrv->IsFileReadOnly(FileName);
			return SQLITE_OK;
		}
	}

	return SQLITE_IOERR_ACCESS;
}
//---------------------------------------------------------------------

static int N2FullPathname(sqlite3_vfs *pVfs, const char *zPath, int nPathOut, char *zPathOut)
{
	nString AbsPath = DataSrv->ManglePath(zPath);

	if (AbsPath.Length() > 1 && AbsPath[1] != ':')
	{
		int Written = (int)GetFullPathName(AbsPath.Get(), nPathOut, zPathOut, NULL);
		n_assert(Written <= nPathOut && Written > 0);
	}
	else
	{
		sqlite3_snprintf(nPathOut, zPathOut, "%s", AbsPath.Get());
		zPathOut[nPathOut - 1] = '\0';
	}

	return SQLITE_OK;
}
//---------------------------------------------------------------------

static void* N2DlOpen(sqlite3_vfs *pVfs, const char *zPath)
{
	return NULL;
}
//---------------------------------------------------------------------

static void N2DlError(sqlite3_vfs *pVfs, int nByte, char *zErrMsg)
{
	sqlite3_snprintf(nByte, zErrMsg, "Loadable extensions are not supported");
	zErrMsg[nByte - 1] = '\0';
}
//---------------------------------------------------------------------

static void (*N2DlSym(sqlite3_vfs *pVfs, void *pH, const char *z))(void)
{
	return NULL;
}
//---------------------------------------------------------------------

static void N2DlClose(sqlite3_vfs *pVfs, void *pHandle)
{
	return;
}
//---------------------------------------------------------------------

static int N2Randomness(sqlite3_vfs *pVfs, int nBuf, char *zBuf)
{
	int n = 0;
	if (sizeof(SYSTEMTIME) <= nBuf - n)
	{
		SYSTEMTIME x;
		GetSystemTime(&x);
		memcpy(&zBuf[n], &x, sizeof(x));
		n += sizeof(x);
	}
	if (sizeof(DWORD) <= nBuf - n)
	{
		DWORD pid = GetCurrentProcessId();
		memcpy(&zBuf[n], &pid, sizeof(pid));
		n += sizeof(pid);
	}
	if (sizeof(DWORD) <= nBuf - n)
	{
		DWORD cnt = GetTickCount();
		memcpy(&zBuf[n], &cnt, sizeof(cnt));
		n += sizeof(cnt);
	}
	if (sizeof(LARGE_INTEGER) <= nBuf - n)
	{
		LARGE_INTEGER i;
		QueryPerformanceCounter(&i);
		memcpy(&zBuf[n], &i, sizeof(i));
		n += sizeof(i);
	}
	return n;
}
//---------------------------------------------------------------------

static int N2Sleep(sqlite3_vfs *pVfs, int microsec)
{
	int msec = (microsec + 999) / 1000;
	Sleep(msec);
	return msec * 1000;
}
//---------------------------------------------------------------------

static int N2CurrentTime(sqlite3_vfs *pVfs, double *prNow)
{
	// FILETIME structure is a 64-bit value representing the number of 
	// 100-nanosecond intervals since January 1, 1601 (= JD 2305813.5). 

	FILETIME ft;
	static const sqlite3_int64 winFiletimeEpoch = 23058135*(sqlite3_int64)8640000;
	static const sqlite3_int64 max32BitValue =
		(sqlite3_int64)2000000000 + (sqlite3_int64)2000000000 + (sqlite3_int64)294967296;

	GetSystemTimeAsFileTime(&ft);

	sqlite3_int64 piNow = winFiletimeEpoch +
		((((sqlite3_int64)ft.dwHighDateTime)*max32BitValue) +
		(sqlite3_int64)ft.dwLowDateTime)/(sqlite3_int64)10000;

	*prNow = piNow / 86400000.0;

	return SQLITE_OK;
}
//---------------------------------------------------------------------

static sqlite3_vfs N2SQLiteVFS = {
	1,							/* iVersion */
	sizeof(CN2File),			/* szOsFile */
	N_MAXPATH,					/* mxPathname */ // MAX_PATH
	NULL,						/* pNext */
	"Nebula2",					/* zName */
	NULL,						/* pAppData */
	N2Open,						/* xOpen */
	N2Delete,					/* xDelete */
	N2Access,					/* xAccess */
	N2FullPathname,				/* xFullPathname */
	N2DlOpen,					/* xDlOpen */
	N2DlError,					/* xDlError */
	N2DlSym,					/* xDlSym */
	N2DlClose,					/* xDlClose */
	N2Randomness,				/* xRandomness */
	N2Sleep,					/* xSleep */
	N2CurrentTime				/* xCurrentTime */
};
//=====================================================================

int RegisterN2SQLiteVFS()
{
	return sqlite3_vfs_register(&N2SQLiteVFS, 1);
}
//---------------------------------------------------------------------

int UnregisterN2SQLiteVFS()
{
	return sqlite3_vfs_unregister(&N2SQLiteVFS);
}
//---------------------------------------------------------------------
