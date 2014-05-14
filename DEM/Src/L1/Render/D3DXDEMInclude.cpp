#include "D3DXDEMInclude.h"
#include <IO/Streams/FileStream.h>

HRESULT CD3DXDEMInclude::Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	IO::CFileStream File;
	CString FilePath = pName;

	// Try absolute path first
	bool Loaded = File.Open(FilePath, IO::SAM_READ, IO::SAP_SEQUENTIAL);

	// Try in shader dir
	if (!Loaded) Loaded = File.Open(ShaderDir + pName, IO::SAM_READ, IO::SAP_SEQUENTIAL);

	// Try in shader root dir
	if (!Loaded) Loaded = File.Open(ShaderRootDir + pName, IO::SAM_READ, IO::SAP_SEQUENTIAL);

	if (!Loaded)
	{
		Core::Log("D3DXDEMInclude: could not open include file '%s' nor\n\t'%s' nor\n\t'%s'!\n",
			pName, (ShaderDir + pName).CStr(), (ShaderRootDir + pName).CStr());
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
