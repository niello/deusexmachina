//------------------------------------------------------------------------------
//  nd3d9shaderinclude.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "kernel/nkernelserver.h"
#include "gfx2/nd3d9shaderinclude.h"
#include <Data/Streams/FileStream.h>

//------------------------------------------------------------------------------
/**
*/
nD3D9ShaderInclude::nD3D9ShaderInclude(const nString& sDir)
{
    this->shaderDir = sDir + "/";
}

//------------------------------------------------------------------------------
/**
*/
HRESULT
nD3D9ShaderInclude::Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
    Data::CFileStream File;

    // open the file
    // try absolute path first
	if (!File.Open(pName, Data::SAM_READ))
    {
        // try in shader dir
        nString filePath = this->shaderDir + pName;
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

//------------------------------------------------------------------------------
/**
*/
HRESULT
nD3D9ShaderInclude::Close(LPCVOID pData)
{
    n_free((void*)pData);
    return S_OK;
}
