#include "gfx2/D3DXNebula2Include.h"
#include <Data/Streams/FileStream.h>

HRESULT CD3DXNebula2Include::Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	Data::CFileStream File;

	// try absolute path first
	if (!File.Open(pName, Data::SAM_READ))
	{
		// try in shader dir
		nString filePath = shaderDir + pName;
		if (!File.Open(filePath, Data::SAM_READ))
		{
			n_printf("nD3D9Shader: could not open include file '%s' nor '%s'!\n", pName, filePath.Get());
			return E_FAIL;
		}
	}

	// allocate data for file and read it
	int fileSize = File.GetSize();
	void* buffer = n_malloc(fileSize);
	if (!buffer)
	{
		File.Close();
		return E_FAIL;
	}
	File.Read(buffer, fileSize);

	*ppData = buffer;
	*pBytes = fileSize;

	File.Close();

	return S_OK;
}
//---------------------------------------------------------------------
