#include "DEMD3DInclude.h"

#include <IO/PathUtils.h>
#include <IO/Streams/FileStream.h>

CDEMD3DInclude::CDEMD3DInclude(const CString& ShdDir, const CString& ShdRootDir):
	ShaderDir(ShdDir),
	ShaderRootDir(ShdRootDir)
{
	ShaderDir.Trim(" \r\n\t\\", false);
	if (ShaderDir.GetLength() && ShaderDir[ShaderDir.GetLength() - 1] != '/') ShaderDir += '/';
	ShaderRootDir.Trim(" \r\n\t\\", false);
	if (ShaderRootDir.GetLength() && ShaderRootDir[ShaderRootDir.GetLength() - 1] != '/') ShaderRootDir += '/';
}
//---------------------------------------------------------------------

HRESULT CDEMD3DInclude::Open(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
{
	// Try absolute path first
	IO::CFileStream File(pFileName);
	bool Loaded = File.Open(IO::SAM_READ, IO::SAP_SEQUENTIAL);

	// Try in shader dir
	if (!Loaded)
	{
		File.SetFileName(ShaderDir + pFileName);
		Loaded = File.Open(IO::SAM_READ, IO::SAP_SEQUENTIAL);
	}

	// Try in shader root dir
	if (!Loaded)
	{
		File.SetFileName(ShaderRootDir + pFileName);
		Loaded = File.Open(IO::SAM_READ, IO::SAP_SEQUENTIAL);
	}

	if (!Loaded)
	{
		Sys::Log("CDEMD3DInclude: could not open include file '%s' nor\n\t'%s' nor\n\t'%s'!\n",
			pFileName, (ShaderDir + pFileName).CStr(), (ShaderRootDir + pFileName).CStr());
		return E_FAIL;
	}

	int FileSize = File.GetSize();
	void* pBuf = n_malloc(FileSize);
	if (!pBuf)
	{
		File.Close();
		return E_FAIL;
	}
	n_assert(File.Read(pBuf, FileSize) == FileSize);

	*ppData = pBuf;
	*pBytes = FileSize;

	File.Close();

	return S_OK;
}
//---------------------------------------------------------------------
