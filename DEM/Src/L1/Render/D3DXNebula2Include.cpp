#include "D3DXNebula2Include.h"
#include <Data/Streams/FileStream.h>

HRESULT CD3DXNebula2Include::Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	Data::CFileStream File;
	nString FilePath = pName;

	// Try absolute path first
	bool Loaded = File.Open(FilePath, Data::SAM_READ, Data::SAP_SEQUENTIAL);

	// Try in shader dir
	if (!Loaded) Loaded = File.Open(ShaderDir + pName, Data::SAM_READ, Data::SAP_SEQUENTIAL);

	// Try in shader root dir
	if (!Loaded) Loaded = File.Open(ShaderRootDir + pName, Data::SAM_READ, Data::SAP_SEQUENTIAL);

	if (!Loaded)
	{
		n_printf("D3DXNebula2Include: could not open include file '%s' nor\n\t'%s' nor\n\t'%s'!\n",
			pName, (ShaderDir + pName).Get(), (ShaderRootDir + pName).Get());
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
